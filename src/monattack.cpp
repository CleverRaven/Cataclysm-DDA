#include "mondeath.h"
#include "monster.h"
#include "game.h"
#include "debug.h"
#include "map.h"
#include "rng.h"
#include "line.h"
#include "bodypart.h"
#include "material.h"
#include "json.h"
#include "speech.h"
#include "messages.h"
#include "sounds.h"
#include "monattack.h"
#include "mondefense.h"
#include "projectile.h"
#include "iuse_actor.h"
#include "weighted_list.h"
#include "mongroup.h"
#include "translations.h"
#include "morale_types.h"
#include "npc.h"
#include "event.h"
#include "ui.h"
#include "itype.h"
#include "vehicle.h"
#include "mapdata.h"
#include "mtype.h"
#include "field.h"
#include "map_iterator.h"
#include <map>

#include <algorithm>

//Used for e^(x) functions
#include <stdio.h>
#include <math.h>

// for loading monster dialogue:
#include <iostream>

#include <limits>  // std::numeric_limits
#define SKIPLINE(stream) stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n')

const mtype_id mon_ant( "mon_ant" );
const mtype_id mon_ant_larva( "mon_ant_larva" );
const mtype_id mon_ant_soldier( "mon_ant_soldier" );
const mtype_id mon_biollante( "mon_biollante" );
const mtype_id mon_blob( "mon_blob" );
const mtype_id mon_blob_brain( "mon_blob_brain" );
const mtype_id mon_blob_large( "mon_blob_large" );
const mtype_id mon_blob_small( "mon_blob_small" );
const mtype_id mon_breather( "mon_breather" );
const mtype_id mon_breather_hub( "mon_breather_hub" );
const mtype_id mon_creeper_hub( "mon_creeper_hub" );
const mtype_id mon_creeper_vine( "mon_creeper_vine" );
const mtype_id mon_dermatik( "mon_dermatik" );
const mtype_id mon_fungal_hedgerow( "mon_fungal_hedgerow" );
const mtype_id mon_fungaloid( "mon_fungaloid" );
const mtype_id mon_fungaloid_young( "mon_fungaloid_young" );
const mtype_id mon_fungal_tendril( "mon_fungal_tendril" );
const mtype_id mon_fungal_wall( "mon_fungal_wall" );
const mtype_id mon_headless_dog_thing( "mon_headless_dog_thing" );
const mtype_id mon_manhack( "mon_manhack" );
const mtype_id mon_shadow( "mon_shadow" );
const mtype_id mon_triffid( "mon_triffid" );
const mtype_id mon_turret_searchlight( "mon_turret_searchlight" );
const mtype_id mon_zombie_dancer( "mon_zombie_dancer" );
const mtype_id mon_zombie_jackson( "mon_zombie_jackson" );

const skill_id skill_melee( "melee" );
const skill_id skill_gun( "gun" );
const skill_id skill_unarmed( "unarmed" );
const skill_id skill_rifle( "rifle" );
const skill_id skill_launcher( "launcher" );

const species_id ZOMBIE( "ZOMBIE" );
const species_id BLOB( "BLOB" );

const efftype_id effect_bite( "bite" );
const efftype_id effect_bleed( "bleed" );
const efftype_id effect_blind( "blind" );
const efftype_id effect_boomered( "boomered" );
const efftype_id effect_controlled( "controlled" );
const efftype_id effect_corroding( "corroding" );
const efftype_id effect_countdown( "countdown" );
const efftype_id effect_darkness( "darkness" );
const efftype_id effect_dazed( "dazed" );
const efftype_id effect_deaf( "deaf" );
const efftype_id effect_dermatik( "dermatik" );
const efftype_id effect_downed( "downed" );
const efftype_id effect_fearparalyze( "fearparalyze" );
const efftype_id effect_fungus( "fungus" );
const efftype_id effect_glowing( "glowing" );
const efftype_id effect_grabbed( "grabbed" );
const efftype_id effect_infected( "infected" );
const efftype_id effect_laserlocked( "laserlocked" );
const efftype_id effect_onfire( "onfire" );
const efftype_id effect_paralyzepoison( "paralyzepoison" );
const efftype_id effect_raising( "raising" );
const efftype_id effect_rat( "rat" );
const efftype_id effect_shrieking( "shrieking" );
const efftype_id effect_slimed( "slimed" );
const efftype_id effect_stunned( "stunned" );
const efftype_id effect_targeted( "targeted" );
const efftype_id effect_teleglow( "teleglow" );

// shared utility functions
int within_visual_range(monster *z, int max_range) {
    int dist;

    dist = rl_dist( z->pos(), g->u.pos() );
    if (dist > max_range || !z->sees( g->u ) ) {
        return -1;    // Out of range
    }
    return dist;
}

bool within_target_range(monster *const z, Creature *const target, int range)
{
    if( target == nullptr ||
        rl_dist( z->pos(), target->pos() ) > range ||
        !z->sees( *target ) ) {
        return false;
    }
    return true;
}

// Distance == 1 and on the same z-level or with a clear shot up/down.
// If allow_zlev is false, don't allow attacking up/down at all.
// If allow_zlev is true, also allow distance == 1 and on different z-level
// as long as floor/ceiling doesn't exist.
bool is_adjacent( const monster *z, const Creature *target, const bool allow_zlev )
{
    if( target == nullptr ) {
        return false;
    }

    if( rl_dist( z->pos(), target->pos() ) != 1 ) {
        return false;
    }

    if( z->posz() == target->posz() ) {
        return true;
    }

    if( !allow_zlev ) {
        return false;
    }

    // The square above must have no floor (currently only open air).
    // The square below must have no ceiling (ie. be outside).
    const bool target_above = target->posz() > z->posz();
    const tripoint &up =   target_above ? target->pos() : z->pos();
    const tripoint &down = target_above ? z->pos() : target->pos();
    return g->m.ter( up ) == t_open_air && g->m.is_outside( down );
}

npc make_fake_npc(monster *z, int str, int dex, int inte, int per) {
    npc tmp;
    tmp.name = _("The ") + z->name();
    tmp.set_fake(true);
    tmp.recoil = 0;
    tmp.setpos( z->pos() );
    tmp.str_cur = str;
    tmp.dex_cur = dex;
    tmp.int_cur = inte;
    tmp.per_cur = per;
    if( z->friendly != 0 ) {
        tmp.attitude = NPCATT_FOLLOW;
    } else {
        tmp.attitude = NPCATT_KILL;
    }
    return tmp;
}

bool mattack::none(monster *)
{
    return true;
}

bool mattack::antqueen(monster *z)
{
    std::vector<tripoint> egg_points;
    std::vector<int> ants;
    // Count up all adjacent tiles the contain at least one egg.
    for( const auto &dest : g->m.points_in_radius( z->pos(), 2 ) ) {
        if( g->m.impassable( dest ) ) {
            continue;
        }

        const int mondex = g->mon_at( dest );
        if( mondex != -1 ) {
            monster &mon = g->zombie( mondex );
            if( mon.type->id == mon_ant_larva ||
                mon.type->id == mon_ant ) {
                ants.push_back(mondex);
            }

            continue;
        }

        if( g->is_empty( dest ) && g->m.has_items( dest ) ) {
            for( auto &i : g->m.i_at( dest ) ) {
                if( i.typeId() == "ant_egg" ) {
                    egg_points.push_back( dest );
                    // Done looking at this tile
                    break;
                }
            }
        }
    }

    if( !ants.empty() ) {
        z->moves -= 100; // It takes a while
        monster *ant = &(g->zombie( random_entry( ants ) ) );
        if( g->u.sees( *z ) && g->u.sees( *ant ) )
            add_msg(m_warning, _("The %1$s feeds an %2$s and it grows!"), z->name().c_str(),
                    ant->name().c_str());
        if (ant->type->id == mon_ant_larva) {
            ant->poly( mon_ant );
        } else {
            ant->poly( mon_ant_soldier );
        }
    } else if (egg_points.empty()) { // There's no eggs nearby--lay one.
        if( g->u.sees( *z ) ) {
            add_msg(_("The %s lays an egg!"), z->name().c_str());
        }
        g->m.spawn_item(z->pos(), "ant_egg", 1, 0, calendar::turn);
    } else { // There are eggs nearby.  Let's hatch some.
        z->moves -= 20 * egg_points.size(); // It takes a while
        if( g->u.sees( *z ) ) {
            add_msg(m_warning, _("The %s tends nearby eggs, and they hatch!"), z->name().c_str());
        }
        for( auto &i : egg_points ) {
            auto eggs = g->m.i_at( i );
            for( size_t j = 0; j < eggs.size(); j++ ) {
                if( eggs[j].typeId() != "ant_egg" ) {
                    continue;
                }
                g->m.i_rem( i, j );
                monster tmp( mon_ant_larva, i );
                tmp.make_ally( z );
                g->add_zombie(tmp);
                break; // Max one hatch per tile
            }
        }
    }

    return true;
}

bool mattack::shriek(monster *z)
{
    Creature *target = z->attack_target();
    if( target == nullptr ||
        rl_dist( z->pos(), target->pos() ) > 4 ||
        !z->sees( *target ) ) {
        return false;
    }

    z->moves -= 240;   // It takes a while
    sounds::sound(z->pos(), 50, _("a terrible shriek!"));
    return true;
}

bool mattack::shriek_alert(monster *z)
{
    if( !z->can_act() || z->has_effect( effect_shrieking ) ) {
        return false;
    }

    Creature *target = z->attack_target();

    int dist;
    if( target == nullptr || (dist = rl_dist( z->pos(), target->pos() )) > 15 ||
        !z->sees( *target ) ) {
        return false;
    }

    if(g->u.sees( *z )){
        add_msg( _("The %s begins shrieking!"), z->name().c_str());
    }

    z->moves -= 150;
    sounds::sound(z->pos(), 120, _("a piercing wail!"));
    z->add_effect( effect_shrieking, 10 );

    return true;
}

bool mattack::shriek_stun(monster *z)
{
    if( !z->can_act() || !z->has_effect( effect_shrieking ) ) {
        return false;
    }

    Creature *target = z->attack_target();
    if( target == nullptr ) {
        return false;
    }

    int dist = rl_dist( z->pos(), target->pos() );
    // Currently the cone is 2D, so don't use it for 3D attacks
    if( dist > 7 ||
        z->posz() != target->posz() ||
        !z->sees( *target ) ) {
        return false;
    }

    int target_angle = g->m.coord_to_angle(z->posx(), z->posy(), target->posx(), target->posy());
    int cone_angle = 20;
    for( const tripoint &cone : g->m.points_in_radius( z->pos(), 4) ) {
        int tile_angle = g->m.coord_to_angle(z->posx(), z->posy(), cone.x, cone.y);
        int diff = abs( target_angle - tile_angle );
        if( diff + cone_angle > 360 || diff > cone_angle || cone == z->pos()) {
            continue; // skip the target, because it's outside cone or it's the source
        }
        // affect the target
        g->m.bash( cone, 4, true ); //Small bash to every square, silent to not flood message box

        Creature *target = g->critter_at( cone ); //If a monster is there, chance for stun
        if ( target == nullptr ){
            continue;
        }
        if ( one_in(dist/2) && !(target->is_immune_effect( effect_deaf ) ) ) {
            target->add_effect( effect_dazed, rng( 10 , 20 ), num_bp, false, rng( 1, ( 15 - dist ) / 3 ) );
        }

    }

    return true;
}

bool mattack::howl(monster *z)
{
    Creature *target = z->attack_target();
    if( target == nullptr ||
        rl_dist( z->pos(), target->pos() ) > 4 ||
        !z->sees( *target ) ) {
        return false;
    }

    z->moves -= 200;   // It takes a while
    sounds::sound(z->pos(), 35, _("an ear-piercing howl!"));

    if( z->friendly != 0 ) { // TODO: Make this use mon's faction when those are in
        for( size_t i = 0; i < g->num_zombies(); ++i ) {
            auto &other = g->zombie( i );
            if( other.is_dead() || other.type != z->type || z->friendly != 0 ) {
                continue;
            }
            // Quote KA101: Chance of friendlying other howlers in the area, I'd imagine:
            // wolves use howls for communication and can convey that the ape is on Team Wolf.
            if( one_in( 4 ) ) {
                other.friendly = z->friendly;
                break;
            }
        }
    }

    return true;
}

bool mattack::rattle(monster *z)
{
    // TODO: Let it rattle at non-player friendlies
    const int min_dist = z->friendly != 0 ? 1 : 4;
    Creature *target = &g->u; // Can't use attack_target - the snake has no target
    if( target == nullptr ||
        rl_dist( z->pos(), target->pos() ) > min_dist ||
        !z->sees( *target ) ) {
        return false;
    }

    z->moves -= 20;   // It takes a very short while
    sounds::sound(z->pos(), 10, _("a sibilant rattling sound!"));

    return true;
}

bool mattack::acid(monster *z)
{
    if( !z->can_act() ) {
        return false;
    }

    Creature *target = z->attack_target();
    if( target == nullptr ) {
        return false;
    }

    if( !z->sees( *target ) ||
        !g->m.clear_path( z->pos(), target->pos(), 10, 1, 100 ) ) {
        return false; // Can't see/reach target, no attack
    }
    z->moves -= 300;   // It takes a while
    sounds::sound(z->pos(), 4, _("a spitting noise."));

    projectile proj;
    proj.speed = 10;
    proj.impact.add_damage( DT_ACID, 5 ); // Mostly just for momentum
    proj.range = 10;
    proj.proj_effects.insert( "NO_OVERSHOOT" );
    auto dealt = z->projectile_attack( proj, target->pos(), 5400 );
    const tripoint &hitp = dealt.end_point;
    const Creature *hit_critter = dealt.hit_critter;
    if( hit_critter == nullptr && g->m.hit_with_acid( hitp ) && g->u.sees( hitp ) ) {
        add_msg( _("A glob of acid hits the %s!"),
                 g->m.tername( hitp ).c_str());
        if( g->m.impassable( hitp ) ) {
            // TODO: Allow it to spill on the side it hit from
            return true;
        }
    }

    for (int i = -3; i <= 3; i++) {
        for (int j = -3; j <= 3; j++) {
            tripoint dest = hitp + tripoint( i, j, 0 );
            if (g->m.passable( dest ) &&
                g->m.clear_path( dest, hitp, 6, 1, 100 ) &&
                ((one_in(abs(j)) && one_in(abs(i))) || (i == 0 && j == 0))) {
                g->m.add_field( dest, fd_acid, 2, 0 );
            }
        }
    }

    return true;
}

bool mattack::acid_barf(monster *z)
{
    if( !z->can_act() ) {
        return false;
    }

    // Let it be used on non-player creatures
    Creature *target = z->attack_target();
    if( target == nullptr || !is_adjacent( z, target, false ) ) {
        return false;
    }

    z->moves -= 80;
    // Make sure it happens before uncanny dodge
    g->m.add_field( target->pos(), fd_acid, 1, 0 );
    bool uncanny = target->uncanny_dodge();
    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if( uncanny || dodge_check(z, target) ){
        auto msg_type = target == &g->u ? m_warning : m_info;
        target->add_msg_player_or_npc( msg_type,
            _("The %s barfs acid at you, but you dodge!"),
            _("The %s barfs acid at <npcname>, but they dodge!"),
            z->name().c_str() );
        if( !uncanny ) {
            target->on_dodge( z, z->type->melee_skill * 2 );
        }

        return true;
    }

    body_part hit = target->get_random_body_part();
    int dam = rng(5, 12);
    dam = target->deal_damage( z, hit, damage_instance( DT_ACID, dam ) ).total_damage();
    target->add_env_effect( effect_corroding, hit, 5, dam / 2 + 5, hit );

    if( dam > 0 ) {
        auto msg_type = target == &g->u ? m_bad : m_info;
        //~ 1$s is monster name, 2$s bodypart in accusative
        target->add_msg_player_or_npc( msg_type,
            _("The %1$s barfs acid on your %2$s for %3$d damage!"),
            _("The %1$s barfs acid on <npcname>'s %2$s for %3$d damage!"),
            z->name().c_str(),
            body_part_name_accusative( hit ).c_str(),
            dam );

        if( hit == bp_eyes ) {
            target->add_env_effect( effect_blind, bp_eyes, 3, 10 );
        }
    } else {
        target->add_msg_player_or_npc(
            _("The %1$s barfs acid on your %2$s, but it washes off the armor!"),
            _("The %1$s bites <npcname>'s %2$s, but it washes off the armor!"),
            z->name().c_str(),
            body_part_name_accusative( hit ).c_str() );
    }

    target->on_hit( z, hit,  z->type->melee_skill );

    return true;
}

bool mattack::acid_accurate(monster *z)
{
    if( !z->can_act() ) {
        return false;
    }

    Creature *target = z->attack_target();
    if( target == nullptr ) {
        return false;
    }

    const int range = rl_dist( z->pos(), target->pos() );
    if( range > 10 || range < 2 || !z->sees( *target ) ) {
        return false;
    }

    z->moves -= 50;

    projectile proj;
    proj.speed = 10;
    proj.range = 10;
    proj.proj_effects.insert( "BLINDS_EYES" );
    proj.proj_effects.insert( "NO_DAMAGE_SCALING" );
    proj.impact.add_damage( DT_ACID, rng( 3, 5 ) );
    // Make it arbitrarily less accurate at close ranges
    z->projectile_attack( proj, target->pos(), rng( 0, 8000 / range ) );

    return true;
}

bool mattack::shockstorm(monster *z)
{
    if( !z->can_act() ) {
        return false;
    }

    Creature *target = z->attack_target();
    if( target == nullptr ) {
        return false;
    }

    bool seen = g->u.sees( *z );
    if( !z->sees( *target ) ||
        !g->m.clear_path( z->pos(), target->pos(), 12, 1, 100 ) ) {
        return false; // Can't see/reach target, no attack
    }

    z->moves -= 50;   // It takes a while

    if( seen ) {
        auto msg_type = target == &g->u ? m_bad : m_neutral;
        add_msg( msg_type, _("A bolt of electricity arcs towards %s!"), target->disp_name().c_str() );
    }
    sfx::play_variant_sound( "fire_gun", "bio_lightning", sfx::get_heard_volume(z->pos()) );
    tripoint tarp( target->posx() + rng(-1, 1) + rng(-1, 1),
                   target->posy() + rng(-1, 1) + rng(-1, 1),
                   target->posz() );
    std::vector<tripoint> bolt = line_to( z->pos(), tarp, 0, 0 );
    for( auto &i : bolt ) { // Fill the LOS with electricity
        if (!one_in(4)) {
            g->m.add_field( i, fd_electricity, rng(1, 3), 0 );
        }
    }
    // 5x5 cloud of electricity at the square hit
    for( const auto &dest : g->m.points_in_radius( tarp, 2 ) ) {
        if( !one_in(4) ) {
            g->m.add_field( dest, fd_electricity, rng(1, 3), 0 );
        }
    }

    return true;
}

bool mattack::pull_metal_weapon(monster *z)
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constants and Configuration

    // max distance that "pull_metal_weapon" can be applied to the target.
    constexpr auto max_distance = 12;

    // attack movement costs
    constexpr int att_cost_pull = 150;

    // minimum str to resist "pull_metal_weapon"
    constexpr int min_str = 4;

    Creature *target = z->attack_target();
    if( target == nullptr ) {
        return false;
    }

    if( !z->sees( *target ) || !g->m.clear_path( z->pos(), target->pos(),
                                                 max_distance, 1, 100 ) ) {
        return false; // Can't see/reach target, no attack
    }
    player *foe = dynamic_cast< player* >( target );
    if( foe != nullptr ) {
        if ( foe->weapon.made_of( material_id( "iron" ) ) || foe->weapon.made_of( material_id( "steel" ) ) ) {
            int wp_skill = foe->get_skill_level( skill_melee );
            z->moves -= att_cost_pull;   // It takes a while
            int success = 100;
            ///\EFFECT_STR increases resistance to pull_metal_weapon special attack
            if ( foe->str_cur > min_str ) {
                ///\EFFECT_MELEE increases resistance to pull_metal_weapon special attack
                success = std::max(100 - (6 * (foe->str_cur - 6)) - (6 * wp_skill), 0);
            }
            auto m_type = foe == &g->u ? m_bad : m_neutral;
            if ( rng(1, 100) <= success ) {
                target->add_msg_player_or_npc( m_type, _("%s is pulled away from your hands!"),
                                                       _("%s is pulled away from <npcname>'s hands!"), foe->weapon.tname().c_str() );
                z->add_item(foe->remove_weapon());
            } else {
                target->add_msg_player_or_npc( m_type, _("The %s unsuccessfully attempts to pull your weapon away."),
                                                       _("The %s unsuccessfully attempts to pull <npcname>'s weapon away."), z->name().c_str() );
            }
        }
    }

    return true;
}

bool mattack::boomer(monster *z)
{
    if( !z->can_act() ) {
        return false;
    }

    Creature *target = z->attack_target();
    if( target == nullptr || rl_dist( z->pos(), target->pos() ) > 3 || !z->sees( *target ) ) {
        return false;
    }

    std::vector<tripoint> line = g->m.find_clear_path( z->pos(), target->pos() );
    z->moves -= 250;   // It takes a while
    bool u_see = g->u.sees( *z );
    if( u_see ) {
        add_msg(m_warning, _("The %s spews bile!"), z->name().c_str());
    }
    for (auto &i : line) {
        g->m.add_field( i, fd_bile, 1, 0 );
        // If bile hit a solid tile, return.
        if (g->m.impassable( i )) {
            g->m.add_field( i, fd_bile, 3, 0 );
            if (g->u.sees( i ))
                add_msg(_("Bile splatters on the %s!"),
                        g->m.tername( i ).c_str());
            return true;
        }
    }
    if( !target->uncanny_dodge() ) {
        ///\EFFECT_DODGE increases chance to avoid boomer effect
        if (rng(0, 10) > target->get_dodge() || one_in( target->get_dodge() ) ) {
            target->add_env_effect( effect_boomered, bp_eyes, 3, 12 );
        } else if( u_see ) {
            target->add_msg_player_or_npc( _("You dodge it!"),
                                           _("<npcname> dodges it!") );
        }
        target->on_dodge( z, 10 );
    }

    return true;
}

bool mattack::boomer_glow(monster *z)
{
    if( !z->can_act() ) {
        return false;
    }

    Creature *target = z->attack_target();
    if( target == nullptr || rl_dist( z->pos(), target->pos() ) > 3 || !z->sees( *target ) ) {
        return false;
    }

    std::vector<tripoint> line = g->m.find_clear_path( z->pos(), target->pos() );
    z->moves -= 250;   // It takes a while
    bool u_see = g->u.sees( *z );
    if( u_see ) {
        add_msg(m_warning, _("The %s spews bile!"), z->name().c_str());
    }
    for (auto &i : line) {
        g->m.add_field(i, fd_bile, 1, 0);
        if (g->m.impassable(i)) {
            g->m.add_field(i, fd_bile, 3, 0);
            if (g->u.sees( i ))
                add_msg(_("Bile splatters on the %s!"), g->m.tername(i).c_str());
            return true;
        }
    }
    if( !target->uncanny_dodge() ) {
        ///\EFFECT_DODGE increases chance to avoid glowing boomer effect
        if (rng(0, 10) > target->get_dodge() || one_in( target->get_dodge() ) ) {
            target->add_env_effect( effect_boomered, bp_eyes, 5, 25 );
            target->on_dodge( z, 10 );
            for (int i = 0; i < rng(2,4); i++){
                body_part bp = random_body_part();
                target->add_env_effect( effect_glowing, bp, 4, 40 );
                if( target->has_effect( effect_glowing ) ) {
                    break;
                }
            }
        } else {
            target->add_msg_player_or_npc( _("You dodge it!"),
                                    _("<npcname> dodges it!") );
        }
    }

    return true;
}


bool mattack::resurrect(monster *z)
{
    // Chance to recover some of our missing speed (yes this will regain
    // loses from being revived ourselves as well).
    // Multiplying by (current base speed / max speed) means that the
    // rate of speed regaining is unaffected by what our current speed is, i.e.
    // we will regain the same amount per minute at speed 50 as speed 200.
    if (one_in(int(15 * double(z->get_speed_base()) / double(z->type->speed)))) {
        // Restore 10% of our current speed, capping at our type maximum
        z->set_speed_base(std::min(z->type->speed, int(z->get_speed_base() + .1 * z->type->speed)));
    }

    int raising_level = 0;
    if( z->has_effect( effect_raising ) ) {
        raising_level = z->get_effect_int( effect_raising ) * 40;
    }

    bool sees_necromancer = g->u.sees(*z);
    std::vector<std::pair<tripoint, item*>> corpses;
    // Find all corpses that we can see within 10 tiles.
    int range = 10;
    bool found_eligible_corpse = false;
    int lowest_raise_score = INT_MAX;
    for( const tripoint &p : g->m.points_in_radius( z->pos(), range ) ) {
        if( !g->is_empty( p ) || g->m.get_field_strength( p, fd_fire ) > 1 ||
            !g->m.sees( z->pos(), p, -1) ) {
            continue;
        }

        for( auto &i : g->m.i_at( p ) ) {
            const mtype *mt = i.get_mtype();
            if( !( i.is_corpse() && i.active && mt->has_flag( MF_REVIVES ) &&
                mt->in_species( ZOMBIE ) && !mt->has_flag( "NO_NECRO" ) ) ) {
                continue;
            }

            found_eligible_corpse = true;
            if( raising_level == 0 ) {
                // Since we have a target, start charging to raise it.
                if( sees_necromancer ) {
                    add_msg(m_info, _("The %s throws its arms wide."), z->name().c_str());
                }
                while( z->moves >= 0 ) {
                    z->add_effect( effect_raising, 10 );
                    z->moves -= 100;
                }
                return false;
            }
            int raise_score = (i.damage() + 1) * mt->hp + i.burnt;
            lowest_raise_score = std::min( lowest_raise_score, raise_score );
            if( raise_score <= raising_level ) {
                corpses.push_back( std::make_pair( p, &i ) );
            }
        }
    }

    if( corpses.empty() ) { // No nearby corpses
        if( found_eligible_corpse ) {
            // There was a corpse, but we haven't charged enough.
            if( sees_necromancer && one_in(sqrt(lowest_raise_score / 30))) {
                add_msg(m_info, _("The %s gesticulates wildly."), z->name().c_str());
            }
            while( z->moves >= 0 ) {
                z->add_effect( effect_raising, 10 );
                z->moves -= 100;
                return false;
            }
        } else if( raising_level != 0 ) {
            z->remove_effect( effect_raising );
        }
        // Check to see if there are any nearby living zombies to see if we should get angry
        bool allies = false;
        for (size_t i = 0; i < g->num_zombies(); i++) {
            monster *zed = &g->zombie(i);
            if( zed != z && zed->type->has_flag(MF_REVIVES) && zed->type->in_species( ZOMBIE ) &&
                  z->attitude_to(*zed) == Creature::Attitude::A_FRIENDLY  &&
                  within_target_range(z, zed, 10)) {
                allies = true;
                break;
            }
        }
        if (!allies) {
            // Nobody around who we could revive, get angry
            z->anger = 100;
        } else {
            // Someone is around who might die and we could revive,
            // calm down.
            z->anger = 5;
        }
        return false;
    } else {
        // We're reviving someone/could revive someone, calm down.
        z->anger = 5;
    }

    if( z->get_speed_base() <= z->type->speed / 2) {
        // We can only resurrect so many times in a time period
        // and we're currently out
        return false;
    }

    std::pair<tripoint, item*> raised = random_entry( corpses );
    float corpse_damage = raised.second->damage();
    // Did we successfully raise something?
    if (g->revive_corpse(raised.first, *raised.second)) {
        g->m.i_rem( raised.first, raised.second );
        if( sees_necromancer ) {
            add_msg(m_info, _("The %s gestures at a nearby corpse."), z->name().c_str());
        }
        z->remove_effect( effect_raising );
        z->moves -= z->type->speed; // Takes one turn
        // Penalize speed by between 10% and 50% based on how damaged the corpse is.
        float speed_penalty = 0.1 + (corpse_damage * 0.1);
        z->set_speed_base(z->get_speed_base() - speed_penalty * z->type->speed);
        const int mondex = g->mon_at(raised.first);
        if( mondex == -1 ) {
            debugmsg( "Misplaced or failed to revive a zombie corpse" );
            return true;
        }

        monster *zed = &g->zombie( mondex );
        zed->make_ally(z);
        if (g->u.sees(*zed)) {
            add_msg(m_warning, _("A nearby %s rises from the dead!"), zed->name().c_str());
        } else if (sees_necromancer) {
            // We saw the necromancer but not the revival
            add_msg(m_info, _("But nothing seems to happen."));
        }
    }

    return true;
}

bool mattack::smash(monster *z)
{
    if( !z->can_act() ) {
        return false;
    }

    Creature *target = z->attack_target();
    if( target == nullptr || !is_adjacent( z, target, false ) ) {
        return false;
    }

    // Costs lots of moves to give you a little bit of a chance to get away.
    z->moves -= 400;

    if( target->uncanny_dodge() ) {
        return true;
    }

    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if( dodge_check( z, target ) ) {
        target->add_msg_player_or_npc( _("The %s takes a powerful swing at you, but you dodge it!"),
                                       _("The %s takes a powerful swing at <npcname>, who dodges it!"),
                                       z->name().c_str() );
        target->on_dodge( z, z->type->melee_skill * 2 );
        return true;
    }

    target->add_msg_player_or_npc( _("A blow from the %1$s sends %2$s flying!"),
                                   _("A blow from the %s sends <npcname> flying!"),
                                   z->name().c_str(), target->disp_name().c_str() );
    // TODO: Make this parabolic
    g->fling_creature( target, g->m.coord_to_angle( z->posx(), z->posy(), target->posx(), target->posy() ),
                       z->type->melee_sides * z->type->melee_dice * 3 );

    return true;
}

//--------------------------------------------------------------------------------------------------
// TODO: move elsewhere
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Find empty spaces around origin within a radius of N.
 *
 * @returns a pair with first  = array<tripoint, area>; area = (2*N + 1)^2.
 *                      second = the number of empty spaces found.
 */
template <size_t N = 1>
std::pair<std::array<tripoint, (2*N + 1)*(2*N + 1)>, size_t>
find_empty_neighbors( const tripoint &origin ) {
    constexpr auto r = static_cast<int>(N);

    const int x_min = origin.x - r;
    const int x_max = origin.x + r;
    const int y_min = origin.y - r;
    const int y_max = origin.y + r;

    std::pair<std::array<tripoint, (2*N + 1)*(2*N + 1)>, size_t> result;

    tripoint tmp;
    tmp.z = origin.z;
    for( tmp.x = x_min; tmp.x <= x_max; ++tmp.x ) {
        for( tmp.y = y_min; tmp.y <= y_max; ++tmp.y ) {
            if (g->is_empty( tmp )) {
                result.first[result.second++] = tmp;
            }
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Find empty spaces around a creature within a radius of N.
 *
 * @see find_empty_neighbors
 */
 template <size_t N = 1>
std::pair<std::array<tripoint, (2*N + 1)*(2*N + 1)>, size_t>
find_empty_neighbors(Creature const &c) {
    return find_empty_neighbors<N>(c.pos());
}

//--------------------------------------------------------------------------------------------------
/**
 * Get a size_t value in the closed interval [0, size]; a convenience to avoid messy casting.
  */
size_t get_random_index(size_t const size) {
    return static_cast<size_t>(rng(0, static_cast<long>(size - 1)));
}

//--------------------------------------------------------------------------------------------------
/**
 * Get a size_t value in the closed interval [0, c.size() - 1]; a convenience to avoid messy casting.
 */
template <typename Container>
size_t get_random_index(Container const &c) {
    return get_random_index(c.size());
}

bool mattack::science(monster *const z) // I said SCIENCE again!
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constants and Configuration

    // attack types
    enum : int {
        att_shock,
        att_radiation,
        att_manhack,
        att_acid_pool,
        att_flavor,
        att_enum_size
    };

    // max distance that "science" can be applied to the target.
    constexpr auto max_distance = 5;

    // attack movement costs
    constexpr int att_cost_shock   = 0;
    constexpr int att_cost_rad     = 400;
    constexpr int att_cost_manhack = 200;
    constexpr int att_cost_acid    = 100;
    constexpr int att_cost_flavor  = 80;

    // radiation attack behaviour
    constexpr int att_rad_dodge_diff    = 16; // how hard it is to dodge
    constexpr int att_rad_mutate_chance = 6;  // (1/x) inverse chance to cause mutation.
    constexpr int att_rad_dose_min      = 20; // min radiation
    constexpr int att_rad_dose_max      = 50; // max radiation

    // acid attack behaviour
    constexpr int att_acid_density = 3;

    // flavor messages
    static std::array<char const*, 4> const m_flavor = {{
        _("The %s gesticulates wildly!"),
        _("The %s coughs up a strange dust."),
        _("The %s moans softly."),
        _("The %s's skin crackles with electricity."), //special case; leave this last
    }};

    if( !z->can_act() ) {
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Look for a valid target...
    Creature *const target = z->attack_target();
    if (!target) {
        return false;
    }

    // too far
    const int dist = rl_dist(z->pos(), target->pos());
    if (dist > max_distance) {
        return false;
    }

    // can't attack what you can't see
    if (!z->sees(*target)) {
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // ok, we have a valid target; populate valid attack options...
    std::array<int, att_enum_size> valid_attacks;
    size_t valid_attack_count = 0;

    // can only shock if adjacent
    if (dist == 1) {
        valid_attacks[valid_attack_count++] = att_shock;
    }

    // TODO: mutate() doesn't like non-players right now
    // It will mutate NPCs, but it will say it mutated the player
    player *const foe = dynamic_cast<player*>(target);
    if ((foe == &g->u) && dist <= 2) {
        valid_attacks[valid_attack_count++] = att_radiation;
    }

    // need an open space for these attacks
    auto const empty_neighbors = find_empty_neighbors(*z);
    size_t const empty_neighbor_count = empty_neighbors.second;

    if (empty_neighbor_count) {
        if( z->ammo["bot_manhack"] > 0 ) {
            valid_attacks[valid_attack_count++] = att_manhack;
        }
        valid_attacks[valid_attack_count++] = att_acid_pool;
    }

    // flavor is always ok
    valid_attacks[valid_attack_count++] = att_flavor;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // choose and do a valid attack
    const int attack_index = get_random_index(valid_attack_count);
    switch (valid_attacks[attack_index]) {
    default :
        DebugLog(D_WARNING, D_GAME) << "Bad enum value in science.";
        break;
    case att_shock :
        z->moves -= att_cost_shock;

        // Just reuse the taze - it's a bit different (shocks torso vs all),
        // but let's go for consistency here
        taze(z, target);
        break;
    case att_radiation : {
        z->moves -= att_cost_rad;

        // if the player can see it
        if (g->u.sees(*z)) {
            // TODO: mutate() doesn't like non-players right now
            add_msg(m_bad, _("The %1$s opens its mouth and a beam shoots towards %2$s!"),
                z->name().c_str(), target->disp_name().c_str());
        }

        // (1) Give the target a chance at an uncanny_dodge.
        // (2) If that fails, always fail to dodge 1 in dodge_skill times.
        // (3) If ok, dodge if dodge_skill > att_rad_dodge_diff.
        // (4) Otherwise, fail 1 in (att_rad_dodge_diff - dodge_skill) times.
        if (foe->uncanny_dodge()) {
            break;
        }

        int const  dodge_skill  = foe->get_dodge();
        bool const critial_fail = one_in(dodge_skill);
        bool const is_trivial   = dodge_skill > att_rad_dodge_diff;

        ///\EFFECT_DODGE increases chance to avoid science effect
        if (!critial_fail && (is_trivial || dodge_skill > rng(0, att_rad_dodge_diff))) {
            target->add_msg_player_or_npc(_("You dodge the beam!"),
                                          _("<npcname> dodges the beam!"));
        } else if (one_in(att_rad_mutate_chance)) {
            foe->mutate();
        } else {
            target->add_msg_if_player(m_bad, _("You get pins and needles all over."));
            foe->radiation += rng(att_rad_dose_min, att_rad_dose_max);
        }
      } break;
    case att_manhack : {
        z->moves -= att_cost_manhack;
        z->ammo["bot_manhack"]--;

        // if the player can see it
        if (g->u.sees(*z)) {
            add_msg(m_warning, _("The %s opens its coat, and a manhack flies out!"),
                z->name().c_str());
        }

        const tripoint where = empty_neighbors.first[get_random_index(empty_neighbor_count)];
        if( g->summon_mon( mon_manhack, where ) ) {
            monster *manhack = g->monster_at( where );
            manhack->make_ally(z);
        }
      } break;
    case att_acid_pool :
        z->moves -= att_cost_acid;

        // if the player can see it
        if (g->u.sees(*z)) {
            add_msg(m_warning, _("The %s drops a flask of acid!"), z->name().c_str());
        }

        // fill empty tiles with acid
        for (size_t i = 0; i < empty_neighbor_count; ++i) {
            const tripoint &p = empty_neighbors.first[i];
            g->m.add_field( p, fd_acid, att_acid_density, 0 );
        }

        break;
    case att_flavor : {
        const size_t i = get_random_index(m_flavor);

        // the special case; see above
        if (i == m_flavor.size() - 1) {
            z->moves -= att_cost_flavor;
        }

        // if the player can see it, else forget about it
        if (g->u.sees(*z)) {
            add_msg(m_warning, m_flavor[i], z->name().c_str());
        }
      } break;
    }

    return true;
}

body_part body_part_hit_by_plant()
{
    body_part hit = num_bp;
    if (one_in(2)) {
        hit = bp_leg_l;
    } else {
        hit = bp_leg_r;
    }
    if (one_in(4)) {
        hit = bp_torso;
    } else if (one_in(2)) {
        if (one_in(2)) {
            hit = bp_foot_l;
        } else {
            hit = bp_foot_r;
        }
    }
    return hit;
}

bool mattack::growplants(monster *z)
{
    for( const auto &p : g->m.points_in_radius( z->pos(), 3 ) ) {
        // TODO: Make this sensible - it can destroy EVERYTHING
        if( !g->m.has_flag("DIGGABLE", p) && one_in(4) ) {
            g->m.ter_set(p, t_dirt);
            continue;
        }

        if( g->m.is_bashable( p ) && one_in(3) ) {
            // Destroy everything
            g->m.destroy( p );
            // And then make the ground fertile
            g->m.ter_set(p, t_dirtmound);
            continue;
        }

        // 1 in 4 chance to grow a tree
        if( !one_in(4) ) {
            if( one_in(3) ) {
                // If no tree, perhaps underbrush
                g->m.ter_set(p, t_underbrush);
            }

            continue;
        }

        // Grow a tree and pierce stuff with it
        Creature *critter = g->critter_at( p );
        // Don't grow under friends (and self)
        if( critter != nullptr &&
            z->attitude_to( *critter ) == Creature::A_FRIENDLY ) {
            continue;
        }

        g->m.ter_set( p, t_tree_young );
        if( critter == nullptr || critter->uncanny_dodge() ) {
            continue;
        }

        const body_part hit = body_part_hit_by_plant();
        //~ %s is bodypart name in accusative.
        critter->add_msg_player_or_npc( m_bad,
            _("A tree bursts forth from the earth and pierces your %s!"),
            _("A tree bursts forth from the earth and pierces <npcname>'s %s!"),
            body_part_name_accusative( hit ).c_str() );
        critter->deal_damage( z, hit, damage_instance( DT_STAB, rng( 10, 30 ) ) );
    }

    // 1 in 5 chance of making existing vegetation grow larger
    if( !one_in(5) ) {
        return true;
    }
    for( const tripoint &p : g->m.points_in_radius( z->pos(), 5 ) ) {
        const auto ter = g->m.ter( p );
        if( ter != t_tree_young && ter != t_underbrush ) {
            // Skip as soon as possible to avoid all the checks
            continue;
        }

        Creature *critter = g->critter_at( p );
        if( critter != nullptr && z->attitude_to( *critter ) == Creature::A_FRIENDLY ) {
            // Don't buff terrain below friends (and self)
            continue;
        }

        if( ter == t_tree_young ) {
            // Young tree => tree
            // TODO: Make this deal damage too - young tree can be walked on, tree can't
            g->m.ter_set( p, t_tree );
        } else if( ter == t_underbrush ) {
            // Underbrush => young tree
            g->m.ter_set( p, t_tree_young );
            if( critter != nullptr && !critter->uncanny_dodge() ) {
                const body_part hit = body_part_hit_by_plant();
                //~ %s is bodypart name in accusative.
                critter->add_msg_player_or_npc( m_bad,
                    _("The underbrush beneath your feet grows and pierces your %s!"),
                    _("Underbrush grows into a tree, and it pierces <npcname>'s %s!"),
                    body_part_name_accusative( hit ).c_str() );
                critter->deal_damage( z, hit, damage_instance( DT_STAB, rng( 10, 30 ) ) );
            }
        }
    }

    return true; // added during refactor, previously had no cooldown reset
}

bool mattack::grow_vine(monster *z)
{
    if( z->friendly ) {
        if( rl_dist( g->u.pos(), z->pos() ) <= 3 ) {
            // Friendly vines keep the area around you free, so you can move.
            return false;
        }
    }
    z->moves -= 100;
    int xshift = rng(0, 2), yshift = rng(0, 2);
    for (int x = 0; x < 3; x++) {
        for (int y = 0; y < 3; y++) {
            tripoint dest( z->posx() + (x + xshift) % 3 - 1,
                           z->posy() + (y + yshift) % 3 - 1,
                           z->posz() );
            if( !g->is_empty( dest ) ) {
                continue;
            }

            if( g->summon_mon( mon_creeper_vine, dest ) ) {
                monster *vine = g->monster_at(dest);
                vine->make_ally( z );
            }
        }
    }

    return true;
}

bool mattack::vine(monster *z)
{
    std::vector<tripoint> grow;
    int vine_neighbors = 0;
    z->moves -= 100;
    for( const tripoint &dest : g->m.points_in_radius( z->pos(), 1 ) ) {
        Creature *critter = g->critter_at( dest );
        if( critter != nullptr && z->attitude_to( *critter ) == Creature::Attitude::A_HOSTILE ) {
            if( critter->uncanny_dodge() ) {
                return true;
            }

            body_part bphit = critter->get_random_body_part();
            critter->add_msg_player_or_npc( m_bad,
                //~ 1$s monster name(vine), 2$s bodypart in accusative
                _("The %1$s lashes your %2$s!"),
                _("The %1$s lashes <npcname>'s %2$s!"),
                z->name().c_str(),
                body_part_name_accusative( bphit ).c_str() );
            damage_instance d;
            // TODO: Buff it to more "modern" numbers - 4+4 is nothing
            d.add_damage( DT_CUT, 4 );
            d.add_damage( DT_BASH, 4 );
            critter->deal_damage( z, bphit, d );
            critter->check_dead_state();
            z->moves -= 100;
            return true;
        }


        if( g->is_empty(dest) ) {
            grow.push_back(dest);
        } else {
            const int zid = g->mon_at(dest);
            if( zid > -1 && g->zombie(zid).type->id == mon_creeper_vine ) {
                vine_neighbors++;
            }
        }
    }
    // Calculate distance from nearest hub
    int dist_from_hub = 999;
    for (size_t i = 0; i < g->num_zombies(); i++) {
        if (g->zombie(i).type->id == mon_creeper_hub) {
            int dist = rl_dist( z->pos(), g->zombie(i).pos() );
            if (dist < dist_from_hub) {
                dist_from_hub = dist;
            }
        }
    }
    if (grow.empty() || vine_neighbors > 5 || one_in(7 - vine_neighbors) ||
        !one_in(dist_from_hub)) {
        return true;
    }
    const tripoint target = random_entry( grow );
    if( g->summon_mon(mon_creeper_vine, target) ) {
        monster *vine = g->monster_at( target );
        vine->make_ally(z);
        vine->reset_special("VINE");
    }

    return true;
}

bool mattack::spit_sap(monster *z)
{
    if( !z->can_act() ) {
        return false;
    }

    Creature *target = z->attack_target();
    if( target == nullptr ||
        rl_dist( z->pos(), target->pos() ) > 12 ||
        !z->sees( *target ) ) {
        return false;
    }

    z->moves -= 150;

    projectile proj;
    proj.speed = 10;
    proj.range = 12;
    proj.proj_effects.insert( "APPLY_SAP" );
    proj.impact.add_damage( DT_ACID, rng( 5, 10 ) );
    z->projectile_attack( proj, target->pos(), 150 );

    return true;
}

bool mattack::triffid_heartbeat(monster *z)
{
    sounds::sound(z->pos(), 14, _("thu-THUMP."));
    z->moves -= 300;
    if( z->friendly != 0 ) {
        return true;
        // TODO: when friendly: open a way to the stairs, don't spawn monsters
    }
    if( g->u.posz() != z->posz() ) {
        // Maybe remove this and allow spawning monsters above?
        return true;
    }

    static pathfinding_settings root_pathfind( 10, 20, 50, false, false, false );
    if (rl_dist( z->pos(), g->u.pos() ) > 5 &&
        !g->m.route( g->u.pos(), z->pos(), root_pathfind ).empty()) {
        add_msg(m_warning, _("The root walls creak around you."));
        for( const tripoint &dest : g->m.points_in_radius( z->pos(), 3 ) ) {
            if (g->is_empty(dest) && one_in(4)) {
                g->m.ter_set(dest, t_root_wall);
            } else if (g->m.ter(dest) == t_root_wall && one_in(10)) {
                g->m.ter_set(dest, t_dirt);
            }
        }
        // Open blank tiles as long as there's no possible route
        int tries = 0;
        while (g->m.route( g->u.pos(), z->pos(), root_pathfind ).empty() &&
               tries < 20) {
            int x = rng(g->u.posx(), z->posx() - 3), y = rng(g->u.posy(), z->posy() - 3);
            tripoint dest( x, y, z->posz() );
            tries++;
            g->m.ter_set(dest, t_dirt);
            if (rl_dist(dest, g->u.pos()) > 3 && g->num_zombies() < 30 &&
                g->mon_at( dest ) == -1 && one_in(20)) { // Spawn an extra monster
                mtype_id montype = mon_triffid;
                if (one_in(4)) {
                    montype = mon_creeper_hub;
                } else if (one_in(3)) {
                    montype = mon_biollante;
                }
                if (g->summon_mon(montype, dest)) {
                    monster *plant = g->monster_at(dest);
                    plant->make_ally(z);
                }
            }
        }

    } else { // The player is close enough for a fight!

        for( const tripoint &dest : g->m.points_in_radius( z->pos(), 1 ) ) {
            if (g->is_empty(dest) && one_in(2)) {
                if (g->summon_mon(mon_triffid, dest)) {
                    monster *triffid = g->monster_at(dest);
                    triffid->make_ally(z);
                }
            }
        }
    }

    return true;
}

bool mattack::fungus(monster *z)
{
    // TODO: Infect NPCs?
    z->moves -= 200;   // It takes a while
    if (g->u.has_trait("THRESH_MYCUS")) {
        z->friendly = 100;
    }
    //~ the sound of a fungus releasing spores
    sounds::sound(z->pos(), 10, _("Pouf!"));
    if (g->u.sees( *z )) {
        add_msg(m_warning, _("Spores are released from the %s!"), z->name().c_str());
    }

    // Use less laggy methods of reproduction when there is a lot of mons around
    double spore_chance = 0.25;
    int radius = 1;
    if( g->num_zombies() > 25 ) {
        // Number of monsters in the bubble and the resulting average number of spores per "Pouf!":
        // 0-25: 2
        // 50  : 0.5
        // 75  : 0.22
        // 100 : 0.125
        // Assuming all monsters in the bubble were fungaloids (unlikely), the average number of spores per generation:
        // 25  : 50
        // 50  : 25
        // 75  : 17
        // 100 : 13
        spore_chance *= ( 25.0 / g->num_zombies() ) * ( 25.0 / g->num_zombies() );
        if( x_in_y( g->num_zombies(), 100 ) ) {
            // Don't make the increased radius spawn more spores
            const double old_area = ( ( 2 * radius + 1 ) * ( 2 * radius + 1 ) ) - 1;
            radius++;
            const double new_area = ( ( 2 * radius + 1 ) * ( 2 * radius + 1 ) ) - 1;
            spore_chance *= old_area / new_area;
        }
    }

    for (int i = -radius; i <= radius; i++) {
        for (int j = -radius; j <= radius; j++) {
            if( i == 0 && j == 0 ) {
                continue;
            }

            tripoint sporep( z->posx() + i, z->posy() + j, z->posz() );
            const int dist = rl_dist( z->pos(), sporep );
            if( !one_in( dist ) ||
                g->m.impassable(sporep) ||
                ( dist > 1 && !g->m.clear_path( z->pos(), sporep, 2, 1, 10 ) ) ) {
                continue;
            }

            g->m.fungalize( sporep, z, spore_chance );
        }
    }

    return true;
}

bool mattack::fungus_haze(monster *z)
{
    //~ That spore sound again
    sounds::sound(z->pos(), 10, _("Pouf!"));
    if (g->u.sees( *z )) {
        add_msg(m_info, _("The %s pulses, and fresh fungal material bursts forth."), z->name().c_str());
    }
    z->moves -= 150;
    for( const tripoint &dest : g->m.points_in_radius( z->pos(), 3 ) ) {
        g->m.add_field( dest, fd_fungal_haze, rng(1, 2), 0 );
    }

    return true;
}

bool mattack::fungus_big_blossom(monster *z)
{
    bool firealarm = false;
    const auto u_see = g->u.sees( *z );
    // Fungal fire-suppressor! >:D
    for( const tripoint &dest : g->m.points_in_radius( z->pos(), 6 ) ) {
        if (g->m.get_field_strength( dest, fd_fire ) != 0) {
            firealarm = true;
        }
        if (firealarm) {
            g->m.remove_field( dest, fd_fire );
            g->m.remove_field( dest, fd_smoke );
            g->m.add_field( dest, fd_fungal_haze, 3, 0 );
        }
    }
    // Special effects handled outside the loop
    if (firealarm){
        if( u_see ) {
            // Sucks up all the smoke
            add_msg(m_warning, _("The %s suddenly inhales!"), z->name().c_str());
        }
        //~Sound of a giant fungal blossom inhaling
        sounds::sound(z->pos(), 20, _("WOOOSH!"));
        if( u_see ) {
            add_msg(m_bad, _("The %s discharges an immense flow of spores, smothering the flames!"), z->name().c_str());
        }
        //~Sound of a giant fungal blossom blowing out the dangerous fire!
        sounds::sound(z->pos(), 20, _("POUFF!"));
        return true;
    }
    // No fire detected, routine haze-emission
    if (!firealarm) {
        //~ That spore sound, much louder
        sounds::sound(z->pos(), 15, _("POUF."));
        if( u_see ) {
            add_msg(m_info, _("The %s pulses, and fresh fungal material bursts forth!"), z->name().c_str());
        }
        z->moves -= 150;
        for( const tripoint &dest : g->m.points_in_radius( z->pos(), 12 ) ) {
            g->m.add_field( dest, fd_fungal_haze, rng(1, 2), 0 );
        }
    }

    return true;
}

bool mattack::fungus_inject(monster *z)
{
    Creature *target = &g->u; // For faster copy+paste
    if (rl_dist( z->pos(), g->u.pos() ) > 1) {
        return false;
    }

    if (g->u.has_trait("THRESH_MARLOSS") || g->u.has_trait("THRESH_MYCUS")) {
        z->friendly = 1;
        return true;
    }
    if ( (g->u.has_trait("MARLOSS")) && (g->u.has_trait("MARLOSS_BLUE")) && !g->u.crossed_threshold()) {
        add_msg(m_info, _("The %s seems to wave you toward the tower..."), z->name().c_str());
        z->anger = 0;
        return true;
    }
    if( z->friendly ) {
        // TODO: attack other creatures, not just g->u, for now just skip the code below as it
        // only attacks g->u but the monster is friendly.
        return true;
    }
    add_msg(m_warning, _("The %s jabs at you with a needlelike point!"), z->name().c_str());
    z->moves -= 150;

    if (g->u.uncanny_dodge()) {
        return true;
    }

    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if (dodge_check(z, target)) {
        target->add_msg_player_or_npc( _("You dodge it!"),
                                       _("<npcname> dodges it!") );
        target->on_dodge( z, z->type->melee_skill * 2  );
        return true;
    }

    body_part hit = target->get_random_body_part();
    int dam = rng(5, 11);
    dam = g->u.deal_damage( z, hit, damage_instance( DT_CUT, dam ) ).total_damage();

    if (dam > 0) {
        //~ 1$s is monster name, 2$s bodypart in accusative
        add_msg(m_bad, _("The %1$s sinks its point into your %2$s!"), z->name().c_str(),
                body_part_name_accusative(hit).c_str());

        if(one_in(10 - dam)) {
            g->u.add_effect( effect_fungus, 100, num_bp, true);
            add_msg(m_warning, _("You feel thousands of live spores pumping into you..."));
        }
    } else {
        //~ 1$s is monster name, 2$s bodypart in accusative
        add_msg(_("The %1$s strikes your %2$s, but your armor protects you."), z->name().c_str(),
                body_part_name_accusative(hit).c_str());
    }

    target->on_hit( z, hit,  z->type->melee_skill  );
    g->u.check_dead_state();

    return true;
}

bool mattack::fungus_bristle(monster *z)
{
    if (g->u.has_trait("THRESH_MARLOSS") || g->u.has_trait("THRESH_MYCUS")) {
        z->friendly = 1;
    }
    Creature *target = z->attack_target();
    if( target == nullptr ||
        !is_adjacent( z, target, true ) ||
        !z->sees( *target ) ) {
        return false;
    }

    auto msg_type = target == &g->u ? m_warning : m_neutral;

    add_msg( msg_type, _("The %1$s swipes at %2$s with a barbed tendril!"), z->name().c_str(), target->disp_name().c_str() );
    z->moves -= 150;

    if( target->uncanny_dodge() ) {
        return true;
    }

    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if (dodge_check(z, target)) {
        target->add_msg_player_or_npc( _("You dodge it!"),
                                       _("<npcname> dodges it!") );
        target->on_dodge( z, z->type->melee_skill * 2 );
        return true;
    }

    body_part hit = target->get_random_body_part();
    int dam = rng(7, 16);
    dam = target->deal_damage( z, hit, damage_instance( DT_CUT, dam ) ).total_damage();

    if( dam > 0 ) {
        //~ 1$s is monster name, 2$s bodypart in accusative
        target->add_msg_if_player( m_bad, _("The %1$s sinks several needlelike barbs into your %2$s!"), z->name().c_str(),
                                body_part_name_accusative(hit).c_str());

        if(one_in(15 - dam)) {
            target->add_effect( effect_fungus, 200, num_bp, true);
            target->add_msg_if_player(m_warning, _("You feel thousands of live spores pumping into you..."));
        }
    } else {
        //~ 1$s is monster name, 2$s bodypart in accusative
        target->add_msg_if_player( _("The %1$s slashes your %2$s, but your armor protects you."), z->name().c_str(),
                                body_part_name_accusative(hit).c_str());
    }

    target->on_hit( z, hit,  z->type->melee_skill );

    return true;
}

bool mattack::fungus_growth(monster *z)
{
    // Young fungaloid growing into an adult
    if (g->u.sees( *z )) {
        add_msg(m_warning, _("The %s grows into an adult!"),
                z->name().c_str());
    }

    z->poly( mon_fungaloid );

    return false;
}

bool mattack::fungus_sprout(monster *z)
{
    bool push_player = false; // To avoid map shift weirdness
    for( const tripoint &dest : g->m.points_in_radius( z->pos(), 1 ) ) {
        if( g->u.pos() == dest ) {
            push_player = true;
        }
        if( g->is_empty(dest) ) {
            if (g->summon_mon(mon_fungal_wall, dest)) {
                monster *wall = g->monster_at(dest);
                wall->make_ally(z);
            }
        }
    }

    if( push_player ) {
        const int angle = g->m.coord_to_angle(z->posx(), z->posy(), g->u.posx(), g->u.posy());
        add_msg(m_bad, _("You're shoved away as a fungal wall grows!"));
        g->fling_creature( &g->u, angle, rng(10, 50) );
    }

    return true;
}

bool mattack::fungus_fortify(monster *z)
{
    if( z->friendly ) {
        return false; // TODO: handle friendly monsters
    }
    Creature *target = &g->u;
    bool mycus = false;
    bool peaceful = true;
    if (g->u.has_trait("THRESH_MARLOSS") || g->u.has_trait("THRESH_MYCUS")) {
        mycus = true; //No nifty support effects.  Yet.  This lets it rebuild hedges.
    }
    if ( (g->u.has_trait("MARLOSS")) && (g->u.has_trait("MARLOSS_BLUE")) &&
         !g->u.crossed_threshold() && !mycus) {
        // You have the other two.  Is it really necessary for us to fight?
        add_msg(m_info, _("The %s spreads its tendrils.  It seems as though it's expecting you..."), z->name().c_str());
        if (rl_dist( z->pos(), g->u.pos() ) < 3) {
            if (query_yn(_("The tower extends and aims several tendrils from its depths.  Hold still?"))) {
                add_msg(m_warning, _("The %s works several tendrils into your arms, legs, torso, and even neck..."), z->name().c_str());
                g->u.hurtall(1, z);
                add_msg(m_warning, _("You see a clear golden liquid pump through the tendrils--and then lose consciousness."));
                g->u.unset_mutation("MARLOSS");
                g->u.unset_mutation("MARLOSS_BLUE");
                g->u.set_mutation("THRESH_MARLOSS");
                g->m.ter_set(g->u.pos(), t_marloss); // We only show you the door.  You walk through it on your own.
                g->u.add_memorial_log(pgettext("memorial_male", "Was shown to the Marloss Gatweay."),
                    pgettext("memorial_female", "Was shown to the Marloss Gateway."));
                g->u.add_msg_if_player(m_good, _("You wake up in a marloss bush.  Almost *cradled* in it, actually, as though it grew there for you."));
                //~ Beginning to hear the Mycus while conscious: this is it speaking
                g->u.add_msg_if_player(m_good, _("assistance, on an arduous quest. unity. together we have reached the door. now to pass through..."));
                return true;
            } else {
                peaceful = false; // You declined the offer.  Fight!
            }
        }
    } else {
        peaceful = false; // You weren't eligible.  Fight!
    }

    bool fortified = false;
    bool push_player = false; // To avoid map shift weirdness
    for( const tripoint &dest : g->m.points_in_radius( z->pos(), 1 ) ) {
        if (g->u.pos() == dest) {
            push_player = true;
        }
        if (g->is_empty(dest)) {
            if (g->summon_mon(mon_fungal_hedgerow, dest)) {
                monster *wall = g->monster_at(dest);
                wall->make_ally(z);
            }
            fortified = true;
        }
    }
    if( push_player ) {
        add_msg(m_bad, _("You're shoved away as a fungal hedgerow grows!"));
        g->fling_creature( &g->u, g->m.coord_to_angle(z->posx(), z->posy(), g->u.posx(),
                           g->u.posy()), rng(10, 50));
    }
    if( fortified || mycus || peaceful ) {
        return true;
    }

    // TODO: De-playerize the whole block
    const int dist = rl_dist( z->pos(), g->u.pos() );
    if( dist >= 12 ) {
        return false;
    }

    if( dist > 3 ) {
        // Oops, can't reach. ):
        // How's about we spawn more tendrils? :)
        // Aimed at the player, too?  Sure!
        const tripoint hit_pos = target->pos() + point( rng( -1, 1 ), rng( -1, 1 ) );
        if( hit_pos == target->pos() && !target->uncanny_dodge() ) {
            const body_part hit = body_part_hit_by_plant();
            //~ %s is bodypart name in accusative.
            add_msg(m_bad, _("A fungal tendril bursts forth from the earth and pierces your %s!"),
                    body_part_name_accusative(hit).c_str());
            g->u.deal_damage( z, hit, damage_instance( DT_CUT, rng( 5, 11 ) ) );
            g->u.check_dead_state();
            // Probably doesn't have spores available *just* yet.  Let's be nice.
        } else if( g->is_empty( hit_pos ) ) {
            add_msg( m_bad, _("A fungal tendril bursts forth from the earth!") );
            if( g->summon_mon(mon_fungal_tendril, hit_pos) ) {
                monster *tendril = g->monster_at( hit_pos );
                tendril->make_ally(z);
            }
        }
        return true;
    }

    add_msg(m_warning, _("The %s takes aim, and spears at you with a massive tendril!"), z->name().c_str());
    z->moves -= 150;

    if( g->u.uncanny_dodge() ) {
        return true;
    }
    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if (dodge_check(z, target)) {
        target->add_msg_player_or_npc( _("You dodge it!"),
                                       _("<npcname> dodges it!") );
        target->on_dodge( z, z->type->melee_skill * 2  );
        return true;
    }

    // TODO: 21 damage with no chance to crit isn't scary
    body_part hit = target->get_random_body_part();
    int dam = rng(15, 21);
    dam = g->u.deal_damage( z, hit, damage_instance( DT_STAB, dam ) ).total_damage();

    if (dam > 0) {
        //~ 1$s is monster name, 2$s bodypart in accusative
        add_msg(m_bad, _("The %1$s sinks its point into your %2$s!"), z->name().c_str(),
            body_part_name_accusative(hit).c_str());
        g->u.add_effect( effect_fungus, 400, num_bp, true);
        add_msg(m_warning, _("You feel millions of live spores pumping into you..."));
        } else {
            //~ 1$s is monster name, 2$s bodypart in accusative
            add_msg(_("The %1$s strikes your %2$s, but your armor protects you."), z->name().c_str(),
                    body_part_name_accusative(hit).c_str());
        }

    target->on_hit( z, hit,  z->type->melee_skill  );
    g->u.check_dead_state();
    return true;
}

bool mattack::impale(monster *z)
{
    if( !z->can_act() ) {
        return false;
    }
    Creature *target = z->attack_target();
    if( target == nullptr || !is_adjacent( z, target, false ) ) {
        return false;
    }

    z->moves -= 80;
    bool uncanny = target->uncanny_dodge();
    if( uncanny || dodge_check(z, target) ){
        auto msg_type = target == &g->u ? m_warning : m_info;
        target->add_msg_player_or_npc( msg_type, _("The %s lunges at you, but you dodge!"),
                                            _("The %s lunges at <npcname>, but they dodge!"),
                                        z->name().c_str() );
        if( !uncanny ) {
            target->on_dodge( z, z->type->melee_skill * 2 );
        }

        return true;
    }

    int dam = target->deal_damage( z, bp_torso, damage_instance( DT_STAB, rng(10,20), rng(5,15), .5 ) ).total_damage();
    if( dam > 0 ) {
        auto msg_type = target == &g->u ? m_bad : m_info;
        //~ 1$s is monster name, 2$s bodypart in accusative
        target->add_msg_player_or_npc( msg_type,
                                    _("The %1$s impales your torso!"),
                                    _("The %1$s impales <npcname>'s torso!"),
                                    z->name().c_str());

        target->on_hit( z, bp_torso,  z->type->melee_skill );
        if( one_in( 60 / (dam + 20)) && (dam > 0) ) {
            target->add_effect( effect_bleed, rng( 75, 125 ), bp_torso, true );
        }

        if( rng(0, 200 + dam) > 100 ) {
            target->add_effect( effect_downed, 3);
        }
        z->moves -= 80; //Takes extra time for the creature to pull out the protrusion
    } else {
        target->add_msg_player_or_npc( _("The %1$s tries to impale your torso, but fails to penetrate your armor!"),
                                     _("The %1$s tries to impale <npcname>'s torso, but fails to penetrate their armor!"),
                                     z->name().c_str());
    }

    target->check_dead_state();

    return true;
}

bool mattack::dermatik(monster *z)
{
    if( !z->can_act() ) {
        return false;
    }

    Creature *target = z->attack_target();
    if( target == nullptr ||
        !is_adjacent( z, target, true ) ||
        !z->sees( *target ) ) {
        return false;
    }

    if( target->uncanny_dodge() ) {
        return true;
    }
    player *foe = dynamic_cast< player* >( target );
    if( foe == nullptr ) {
        return true; // No implanting monsters for now
    }
    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if (dodge_check(z, target)) {
        if( target == &g->u ) {
            add_msg(_("The %s tries to land on you, but you dodge."), z->name().c_str());
        }
        z->stumble();
        target->on_dodge( z, z->type->melee_skill * 2 );
        return true;
    }

    // Can we swat the bug away?
    int dodge_roll = z->dodge_roll();
    ///\EFFECT_MELEE increases chance to deflect dermatik attack

    ///\EFFECT_UNARMED increases chance to deflect dermatik attack
    int swat_skill = ( foe->get_skill_level( skill_melee ) + foe->get_skill_level( skill_unarmed ) * 2) / 3;
    int player_swat = dice(swat_skill, 10);
    if( foe->has_trait("TAIL_CATTLE") ) {
        target->add_msg_if_player(_("You swat at the %s with your tail!"), z->name().c_str());
        ///\EFFECT_DEX increases chance of deflecting dermatik attack with TAIL_CATTLE

        ///\EFFECT_UNARMED increases chance of deflecting dermatik attack with TAIL_CATTLE
        player_swat += ( ( foe->dex_cur + foe->get_skill_level( skill_unarmed ) ) / 2 );
    }
    if( player_swat > dodge_roll ) {
        target->add_msg_if_player(_("The %s lands on you, but you swat it off."), z->name().c_str());
        if (z->get_hp() >= z->get_hp_max() / 2) {
            z->apply_damage( &g->u, bp_torso, 1 );
            z->check_dead_state();
        }
        if (player_swat > dodge_roll * 1.5) {
            z->stumble();
        }
        return true;
    }

    // Can the bug penetrate our armor?
    body_part targeted = target->get_random_body_part();
    if (4 < g->u.get_armor_cut(targeted) / 3) {
        //~ 1$s monster name(dermatic), 2$s bodypart name in accusative.
        target->add_msg_if_player( _("The %1$s lands on your %2$s, but can't penetrate your armor."),
                                z->name().c_str(), body_part_name_accusative(targeted).c_str());
        z->moves -= 150; // Attempted laying takes a while
        return true;
    }

    // Success!
    z->moves -= 500; // Successful laying takes a long time
    //~ 1$s monster name(dermatic), 2$s bodypart name in accusative.
    target->add_msg_if_player( m_bad, _("The %1$s sinks its ovipositor into your %2$s!"), z->name().c_str(),
                            body_part_name_accusative(targeted).c_str());
    if ( !foe->has_trait("PARAIMMUNE") || !foe->has_trait("ACIDBLOOD") ) {
        foe->add_effect( effect_dermatik, 1, targeted, true);
        foe->add_memorial_log(pgettext("memorial_male", "Injected with dermatik eggs."),
                              pgettext("memorial_female", "Injected with dermatik eggs."));
    }

    return true;
}

bool mattack::dermatik_growth(monster *z)
{
    // Dermatik larva growing into an adult
    if (g->u.sees( *z )) {
        add_msg(m_warning, _("The %s dermatik larva grows into an adult!"),
                z->name().c_str());
    }
    z->poly( mon_dermatik );

    return false;
}

bool mattack::plant( monster *z)
{
    // Spores taking seed and growing into a fungaloid
    if( !g->spread_fungus( z->pos() ) && one_in( 10 + g->num_zombies() / 5 ) ) {
        if( g->u.sees( *z ) ) {
            add_msg(m_warning, _("The %s takes seed and becomes a young fungaloid!"),
                    z->name().c_str());
        }

        z->poly( mon_fungaloid_young );
        z->moves -= 1000; // It takes a while
        return false;
    } else {
        if (g->u.sees( *z )) {
            add_msg(_("The %s falls to the ground and bursts!"),
                    z->name().c_str());
        }
        z->set_hp( 0 );
        // Try fungifying once again
        g->spread_fungus( z->pos() );
        return true;
    }
}

bool mattack::disappear(monster *z)
{
    z->set_hp( 0 );
    return true;
}

static void poly_keep_speed( monster &mon, const mtype_id& id )
{
    // Retain old speed after polymorph
    // This prevents blobs regenerating speed through polymorphs
    // and thus replicating indefinitely, covering entire map
    const int old_speed = mon.get_speed_base();
    mon.poly( id );
    mon.set_speed_base( old_speed );
}

static bool blobify( monster &blob, monster &target )
{
    if( g->u.sees( target ) ) {
        add_msg( m_warning, _("%s is engulfed by %s!"),
            target.disp_name().c_str(), blob.disp_name().c_str() );
    }

    switch( target.get_size() ) {
        case MS_TINY:
            // Just consume it
            target.set_hp( 0 );
            blob.set_speed_base( blob.get_speed_base() + 5 );
            return false;
        case MS_SMALL:
            target.poly( mon_blob_small );
            break;
        case MS_MEDIUM:
            target.poly( mon_blob );
            break;
        case MS_LARGE:
            target.poly( mon_blob_large );
            break;
        case MS_HUGE:
            // No polymorphing huge stuff
            target.add_effect( effect_slimed, rng( 2, 10 ) );
            break;
        default:
            debugmsg("Tried to blobify %s with invalid size: %d",
                 target.disp_name().c_str(), (int)target.get_size() );
            return false;
    }

    target.make_ally( &blob );
    return true;
}

bool mattack::formblob(monster *z)
{
    if( z->friendly ) {
        return false; // TODO: handle friendly monsters
    }

    bool didit = false;
    auto pts = closest_tripoints_first( 1, z->pos() );
    // Don't check own tile
    pts.erase( pts.begin() );
    for( const tripoint &dest : pts ) {
        Creature *critter = g->critter_at( dest );
        if( critter == nullptr ) {
            if( z->get_speed_base() > 85 && rng(0, 250) < z->get_speed_base() ) {
                // If we're big enough, spawn a baby blob.
                didit = true;
                z->set_speed_base( z->get_speed_base() - 15 );
                if( g->summon_mon( mon_blob_small, dest ) ) {
                    monster *blob = g->monster_at( dest );
                    blob->make_ally( z );
                }

                break;
            }

            continue;
        }

        monster *mon = dynamic_cast<monster*>( critter );
        if( mon == nullptr ) {
            // If we hit the player or some NPC, cover them with slime
            didit = true;
            // TODO: Add some sort of a resistance/dodge roll
            g->u.add_effect( effect_slimed, rng( 0, z->get_hp() ) );
            break;
        }

        monster &othermon = *mon;
        // Hit a monster.  If it's a blob, give it our speed.  Otherwise, blobify it?
        if( z->get_speed_base() > 40 && othermon.type->in_species( BLOB ) ) {
            if( othermon.type->id == mon_blob_brain ) {
                // Brain blobs don't get sped up, they heal at the cost of the other blob.
                // But only if they are hurt badly.
                if( othermon.get_hp() < othermon.get_hp_max() / 2 ) {
                    didit = true;
                    othermon.heal( z->get_speed_base(), true );
                    z->set_hp( 0 );
                    return true;
                }
                continue;
            }
            didit = true;
            othermon.set_speed_base( othermon.get_speed_base() + 5 );
            z->set_speed_base( z->get_speed_base() - 5 );
            if( othermon.type->id == mon_blob_small && othermon.get_speed_base() >= 60 ) {
                poly_keep_speed( othermon, mon_blob );
            } else if( othermon.type->id == mon_blob && othermon.get_speed_base() >= 80 ) {
                poly_keep_speed( othermon, mon_blob_large );
            }
        } else if( (othermon.made_of( material_id( "flesh" ) ) ||
                    othermon.made_of( material_id( "veggy" ) ) ||
                    othermon.made_of( material_id( "iflesh" ) ) ) &&
                   rng( 0, z->get_hp() ) > rng( othermon.get_hp() / 2, othermon.get_hp() ) ) {
            didit = blobify( *z, othermon );
        }
    }

    if( didit ) { // We did SOMEthing.
        if( z->type->id == mon_blob && z->get_speed_base() <= 50 ) {
            // We shrank!
            poly_keep_speed( *z, mon_blob_small );
        } else if( z->type->id == mon_blob_large && z->get_speed_base() <= 70 ) {
            // We shrank!
            poly_keep_speed( *z, mon_blob );
        }

        z->moves = 0;
        return true;
    }

    return true; // consider returning false to try again immediately if nothing happened?
}

bool mattack::callblobs(monster *z)
{
    if( z->friendly ) {
        return false; // TODO: handle friendly monsters
    }
    // The huge brain blob interposes other blobs between it and any threat.
    // For the moment just target the player, this gets a bit more complicated
    // if we want to deal with NPCS and friendly monsters as well.
    // The strategy is to send about 1/3 of the available blobs after the player,
    // and keep the rest near the brain blob for protection.
    tripoint enemy = g->u.pos();
    std::list<monster *> allies;
    std::vector<tripoint> nearby_points = closest_tripoints_first( 3, z->pos() );
    // Iterate using horrible creature_tracker API.
    for( size_t i = 0; i < g->num_zombies(); i++ ) {
        monster *candidate = &g->zombie( i );
        if( candidate->type->in_species( BLOB ) && candidate->type->id != mon_blob_brain ) {
            // Just give the allies consistent assignments.
            // Don't worry about trying to make the orders optimal.
            allies.push_back( candidate );
        }
    }
    // 1/3 of the available blobs, unless they would fill the entire area near the brain.
    const int num_guards = std::min( allies.size() / 3, nearby_points.size() );
    int guards = 0;
    for( std::list<monster *>::iterator ally = allies.begin();
         ally != allies.end(); ++ally, ++guards ) {
        tripoint post = enemy;
        if( guards < num_guards ) {
            // Each guard is assigned a spot in the nearby_points vector based on their order.
            int assigned_spot = (nearby_points.size() * guards) / num_guards;
            post = nearby_points[ assigned_spot ];
        }
        (*ally)->set_dest( post );
        if (!(*ally)->has_effect( effect_controlled )) {
            (*ally)->add_effect( effect_controlled, 1, num_bp, true);
        }
    }
    // This is telepathy, doesn't take any moves.

    return true;
}

bool mattack::jackson(monster *z)
{
    // Jackson draws nearby zombies into the dance.
    std::list<monster *> allies;
    std::vector<tripoint> nearby_points = closest_tripoints_first( 3, z->pos() );
    // Iterate using horrible creature_tracker API.
    for( size_t i = 0; i < g->num_zombies(); i++ ) {
        monster *candidate = &g->zombie( i );
        if(candidate->type->in_species( ZOMBIE ) && candidate->type->id != mon_zombie_jackson) {
            // Just give the allies consistent assignments.
            // Don't worry about trying to make the orders optimal.
            allies.push_back( candidate );
        }
    }
    const int num_dancers = std::min( allies.size(), nearby_points.size() );
    int dancers = 0;
    bool converted = false;
    for( auto ally = allies.begin(); ally != allies.end(); ++ally, ++dancers ) {
        tripoint post = z->pos();
        if( dancers < num_dancers ) {
            // Each dancer is assigned a spot in the nearby_points vector based on their order.
            int assigned_spot = (nearby_points.size() * dancers) / num_dancers;
            post = nearby_points[ assigned_spot ];
        }
        if ((*ally)->type->id != mon_zombie_dancer) {
            (*ally)->poly( mon_zombie_dancer );
            converted = true;
        }
        (*ally)->set_dest( post );
        if (!(*ally)->has_effect( effect_controlled )) {
            (*ally)->add_effect( effect_controlled, 1, num_bp, true);
        }
    }
    // Did we convert anybody?
    if (converted) {
        if (g->u.sees( *z )) {
            add_msg(m_warning, _("The %s lets out a high-pitched cry!"), z->name().c_str());
        }
    }
    // This is telepathy, doesn't take any moves.

    return true;
}


bool mattack::dance(monster *z)
{
    if (g->u.sees( *z )) {
        switch (rng(1,10)) {
            case 1:
                add_msg(m_neutral, _("The %s swings its arms from side to side!"), z->name().c_str());
                break;
            case 2:
                add_msg(m_neutral, _("The %s does some fancy footwork!"), z->name().c_str());
                break;
            case 3:
                add_msg(m_neutral, _("The %s shrugs its shoulders!"), z->name().c_str());
                break;
            case 4:
                add_msg(m_neutral, _("The %s spins in place!"), z->name().c_str());
                break;
            case 5:
                add_msg(m_neutral, _("The %s crouches on the ground!"), z->name().c_str());
                break;
            case 6:
                add_msg(m_neutral, _("The %s looks left and right!"), z->name().c_str());
                break;
            case 7:
                add_msg(m_neutral, _("The %s jumps back and forth!"), z->name().c_str());
                break;
            case 8:
                add_msg(m_neutral, _("The %s raises its arms in the air!"), z->name().c_str());
                break;
            case 9:
                add_msg(m_neutral, _("The %s swings its hips!"), z->name().c_str());
                break;
            case 10:
                add_msg(m_neutral, _("The %s claps!"), z->name().c_str());
                break;
        }
    }

    return true;
}

bool mattack::dogthing(monster *z)
{
    if( z == nullptr ) {
        return false; // TODO: replace pointers with references
    }

    if (!one_in(3) || !g->u.sees(*z)) {
        return false;
    }

    add_msg(_("The %s's head explodes in a mass of roiling tentacles!"),
            z->name().c_str());

    g->m.add_splash( z->bloodType(), z->pos(), 2, 3 );

    z->friendly = 0;
    z->poly( mon_headless_dog_thing );

    return false;
}

bool mattack::tentacle(monster *z)
{
    if( z->friendly ) {
        return false; // TODO: handle friendly monsters
    }
    Creature *target = &g->u;
    if (!z->sees( g->u )) {
        return false;
    }
    add_msg(m_bad, _("The %s lashes its tentacle at you!"), z->name().c_str());
    z->moves -= 100;

    if (g->u.uncanny_dodge()) {
        return true;
    }
    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if (dodge_check(z, target)) {
        target->add_msg_player_or_npc( _("You dodge it!"),
                                       _("<npcname> dodges it!") );
        target->on_dodge( z, z->type->melee_skill * 2  );
        return true;
    }

    body_part hit = target->get_random_body_part();
    int dam = rng(10, 20);
    //~ 1$s is bodypart name, 2$d is damage value.
    add_msg(m_bad, _("Your %1$s is hit for %2$d damage!"), body_part_name(hit).c_str(), dam);
    g->u.deal_damage( z, hit, damage_instance( DT_BASH, dam ) );
    target->on_hit( z, hit,  z->type->melee_skill  );
    g->u.check_dead_state();

    return true;
}


bool mattack::ranged_pull(monster *z)
{
    Creature *target = z->attack_target();
    if( target == nullptr || rl_dist( z->pos(), target->pos() ) > 3 ||
        rl_dist( z->pos(), target->pos() ) <= 1 || !z->sees( *target ) ) {
        return false;
    }

    player *foe = dynamic_cast< player* >( target );
    std::vector<tripoint> line = g->m.find_clear_path( z->pos(), target->pos() );
    bool seen = g->u.sees( *z );

    for( auto &i : line ) {
        // Player can't be pulled though bars, furniture, cars or creatures
        // TODO: Add bashing? Currently a window is enough to prevent grabbing
        if( !g->is_empty( i ) && i != z->pos() && i != target->pos() ) {
            return false;
        }
    }

    z->moves -= 150;

    const bool uncanny = target->uncanny_dodge();
    if( uncanny || dodge_check(z, target) ) {
        z->moves -= 200;
        auto msg_type = foe == &g->u ? m_warning : m_info;
        target->add_msg_player_or_npc( msg_type, _("The %s's arms fly out at you, but you dodge!"),
                                                 _("The %s's arms fly out at <npcname>, but they dodge!"),
                                       z->name().c_str() );

        if( !uncanny ) {
            target->on_dodge( z, z->type->melee_skill * 2 );
        }

        return true;
    }

    // Limit the range in case some weird math thing would cause the target to fly past us
    int range = std::min( (z->type->melee_sides * z->type->melee_dice) / 10,
                          rl_dist( z->pos(), target->pos() ) + 1 );
    tripoint pt = target->pos();
    while( range > 0 ) {
        // Recalculate the ray each step
        // We can't depend on either the target position being constant (obviously),
        // but neither on z pos staying constant, because we may want to shift the map mid-pull
        const int dir = g->m.coord_to_angle( target->posx(), target->posy(), z->posx(), z->posy() );
        tileray tdir(dir);
        tdir.advance();
        pt.x = target->posx() + tdir.dx();
        pt.y = target->posy() + tdir.dy();
        if( !g->is_empty( pt ) ) { //Cancel the grab if the space is occupied by something
            break;
        }

        if( foe != nullptr ) {
            if( foe->in_vehicle ) {
                g->m.unboard_vehicle( foe->pos() );
            }

            if( target->is_player() && ( pt.x < SEEX * int(MAPSIZE / 2) || pt.y < SEEY * int(MAPSIZE / 2) ||
                pt.x >= SEEX * (1 + int(MAPSIZE / 2)) || pt.y >= SEEY * (1 + int(MAPSIZE / 2)) ) ) {
                g->update_map( pt.x, pt.y );
            }
        }

        target->setpos( pt );
        range--;
        if( target->is_player() && seen ) {
            g->draw();
        }
    }
    if( seen ) {
        add_msg( _("The %1$s's arms fly out and pull and grab %2$s!"), z->name().c_str(), target->disp_name().c_str() );
    }

    const int prev_effect = target->get_effect_int( effect_grabbed );
    target->add_effect( effect_grabbed, 2, bp_torso, false, prev_effect + 4); //Duration needs to be at least 2, or grab will imediately be removed

    return true;
}

bool mattack::grab(monster *z)
{
    if( !z->can_act() ) {
        return false;
    }
    Creature *target = z->attack_target();
    if( target == nullptr || !is_adjacent(z, target, false)) {
        return false;
    }

    z->moves -= 80;
    const bool uncanny = target->uncanny_dodge();
    const auto msg_type = target == &g->u ? m_warning : m_info;
    if( uncanny || dodge_check(z, target) ){
        target->add_msg_player_or_npc( msg_type, _("The %s gropes at you, but you dodge!"),
                                            _("The %s gropes at <npcname>, but they dodge!"),
                                        z->name().c_str() );

        if( !uncanny ) {
            target->on_dodge( z, z->type->melee_skill * 2 );
        }

        return true;
    }

    player *pl = dynamic_cast<player*>( target );
    if( pl == nullptr ) {
        return true;
    }

    ///\EFFECT_DEX increases chance to avoid being grabbed if DEX>STR

    ///\EFFECT_STR increases chance to avoid being grabbed if STR>DEX
    if ( pl->has_grab_break_tec() && pl->get_grab_resist() > 0 && pl->get_dex() > pl->get_str() ?
        rng(0, pl->get_dex()) : rng( 0, pl->get_str()) > rng( 0 , z->type->melee_sides + z->type->melee_dice)) {
        if (target->has_effect( effect_grabbed )){
            target->add_msg_if_player(m_info,_("The %s tries to grab you as well, but you bat it away!"),
                                      z->name().c_str());
        } else {target->add_msg_player_or_npc(m_info, _("The %s tries to grab you, but you break its grab!"),
                                    _("The %s tries to grab <npcname>, but they break its grab!"),
                                    z->name().c_str());
        }
        return true;
    }

    const int prev_effect = target->get_effect_int( effect_grabbed );
    target->add_effect( effect_grabbed, 2, bp_torso, false, prev_effect + 1);
    target->add_msg_player_or_npc( m_bad, _("The %s grabs you!"), _("The %s grabs <npcname>!"),
                                   z->name().c_str() );

    return true;
}

bool mattack::grab_drag(monster *z)
{
    if( !z->can_act() ) {
        return false;
    }
    Creature *target = z->attack_target();
    monster *zz = dynamic_cast<monster*>(target);
    if( target == nullptr || rl_dist( z->pos(), target->pos() ) > 1 ) {
        return false;
    }

    player *foe = dynamic_cast< player* >( target );

    grab(z); //First, grab the target

    if (!target->has_effect( effect_grabbed )){ //Can't drag if isn't grabbed, otherwise try and move
        return false;
    }
    tripoint target_square = z->pos() - (target->pos() - z->pos());
    if (z->can_move_to(target_square) && target->stability_roll() < dice(z->type->melee_sides, z->type->melee_dice) ) {
        tripoint zpt = z->pos();
        z->move_to(target_square);
        if (!g->is_empty(zpt)){ //Cancel the grab if the space is occupied by something
            return false;
        }
        if( target->is_player() && ( zpt.x < SEEX * int(MAPSIZE / 2) || zpt.y < SEEY * int(MAPSIZE / 2) ||
            zpt.x >= SEEX * (1 + int(MAPSIZE / 2)) || zpt.y >= SEEY * (1 + int(MAPSIZE / 2)) ) ) {
            g->update_map( zpt.x, zpt.y );
        }
        if (foe != nullptr){
            if (foe->in_vehicle) {
                g->m.unboard_vehicle(foe->pos());
            }
            foe->setpos(zpt);
        } else {
            zz->setpos(zpt);
        }
        target->add_msg_player_or_npc(m_good, _("You are dragged behind the %s!"),
                                _("<npcname> gets dragged behind the %s!"), z->name().c_str() );
    } else{
        target->add_msg_player_or_npc(m_good, _("You resist the %s as it tries to drag you!"),
                                _("<npcname> resist the %s as it tries to drag them!"), z->name().c_str() );
    }
    int prev_effect = target->get_effect_int( effect_grabbed );
    target->add_effect( effect_grabbed, 2, bp_torso, false, prev_effect + 3);

    return true; // cooldown was not reset prior to refactor here
}

bool mattack::gene_sting(monster *z)
{
    if( z->friendly ) {
        return false; // TODO: handle friendly monsters
    }
    if (within_visual_range(z, 7) < 0) {
        return false;
    }

    z->moves -= 150;

    if (g->u.uncanny_dodge()) {
        return true;
    }
    add_msg(m_bad, _("The %s shoots a dart into you!"), z->name().c_str());
    g->u.mutate();

    return true;
}

bool mattack::para_sting(monster *z)
{
    Creature *target = z->attack_target();
    if( target == nullptr ) {
        return false;
    }
    if( rl_dist( z->pos(), target->pos() ) > 4 ) {
        return false;
    }

    z->moves -= 150;

    if( target->uncanny_dodge() ) {
        return true;
    }
    target->add_msg_if_player(m_bad, _("The %s shoots a dart into you!"), z->name().c_str());
    target->add_msg_if_player(m_bad, _("You feel poison enter your body!"));
    target->add_effect( effect_paralyzepoison, 50);

    return true;
}

bool mattack::triffid_growth(monster *z)
{
    // Young triffid growing into an adult
    if (g->u.sees( *z )) {
        add_msg(m_warning, _("The %s young triffid grows into an adult!"),
                z->name().c_str());
    }
    z->poly( mon_triffid );

    return false;
}

bool mattack::stare(monster *z)
{
    if( z->friendly ) {
        return false; // TODO: handle friendly monsters
    }
    z->moves -= 200;
    if( z->sees( g->u ) ) {
        if( g->u.sees(*z) ) {
            add_msg(m_bad, _("The %s stares at you, and you shudder."), z->name().c_str());
        } else {
            add_msg(m_bad, _("You feel like you're being watched, it makes you sick."));
        }
        g->u.add_effect( effect_teleglow, 800);
    }

    return true;
}

bool mattack::fear_paralyze(monster *z)
{
    if( z->friendly ) {
        return false; // TODO: handle friendly monsters
    }
    if ( g->u.sees( *z ) && !g->u.has_effect( effect_fearparalyze ) ) {
        if (g->u.has_artifact_with(AEP_PSYSHIELD) || (g->u.is_wearing("tinfoil_hat") && one_in(4))) {
            add_msg(_("The %s probes your mind, but is rebuffed!"), z->name().c_str());
        ///\EFFECT_INT decreases chance of being paralyzed by fear attack
        } else if ( rng(0, 20) > g->u.get_int() ) {
            add_msg( m_bad, _("The terrifying visage of the %s paralyzes you."), z->name().c_str() );
            g->u.add_effect( effect_fearparalyze, 5 );
            g->u.moves -= 400;
        } else
            add_msg( _("You manage to avoid staring at the horrendous %s."), z->name().c_str() );
    }

    return true;
}

bool mattack::photograph(monster *z)
{
    if( within_visual_range(z, 6) < 0 ) {
        return false;
    }

    // Badges should NOT be swappable between roles.
    // Hence separate checking.
    // If you are in fact listed as a police officer
    if (g->u.has_trait("PROF_POLICE")) {
        // And you're wearing your badge
        if (g->u.is_wearing("badge_deputy")) {
            if (one_in(3)) {
                add_msg(m_info, _("The %s flashes a LED and departs.  Human officer on scene."), z->name().c_str());
                z->no_corpse_quiet = true;
                z->no_extra_death_drops = true;
                z->die(nullptr);
                return false;
            } else {
                add_msg(m_info, _("The %s acknowledges you as an officer responding, but hangs around to watch."), z->name().c_str());
                add_msg(m_info, _("Probably some now-obsolete Internal Affairs subroutine..."));
                return true;
            }
        }
    }

    if (g->u.has_trait("PROF_PD_DET")) {
        // And you have your shield on
        if (g->u.is_wearing("badge_detective")) {
            if (one_in(4)) {
                add_msg(m_info, _("The %s flashes a LED and departs.  Human officer on scene."), z->name().c_str());
                z->no_corpse_quiet = true;
                z->no_extra_death_drops = true;
                z->die(nullptr);
                return false;
            } else {
                add_msg(m_info, _("The %s acknowledges you as an officer responding, but hangs around to watch."), z->name().c_str());
                add_msg(m_info, _("Ops used to do that in case you needed backup..."));
                return true;
            }
        }
    } else if (g->u.has_trait("PROF_SWAT")) {
        // And you're wearing your badge
        if (g->u.is_wearing("badge_swat")) {
            if (one_in(3)) {
                add_msg(m_info, _("The %s flashes a LED and departs.  SWAT's working the area."), z->name().c_str());
                z->no_corpse_quiet = true;
                z->no_extra_death_drops = true;
                z->die(nullptr);
                return false;
            } else {
                add_msg(m_info, _("The %s acknowledges you as SWAT onsite, but hangs around to watch."), z->name().c_str());
                add_msg(m_info, _("Probably some now-obsolete Internal Affairs subroutine..."));
                return true;
            }
        }
    } else if (g->u.has_trait("PROF_CYBERCOP")) {
        // And you're wearing your badge
        if (g->u.is_wearing("badge_cybercop")) {
            if (one_in(3)) {
                add_msg(m_info, _("The %s winks a LED and departs.  One machine to another?"), z->name().c_str());
                z->no_corpse_quiet = true;
                z->no_extra_death_drops = true;
                z->die(nullptr);
                return false;
            } else {
                add_msg(m_info, _("The %s acknowledges you as an officer responding, but hangs around to watch."), z->name().c_str());
                add_msg(m_info, _("Apparently yours aren't the only systems kept alive post-apocalypse."));
                return true;
            }
        }
    }

    if (g->u.has_trait("PROF_FED")) {
        // And you're wearing your badge
        if (g->u.is_wearing("badge_marshal")) {
            add_msg(m_info, _("The %s flashes a LED and departs.  The Feds have this."), z->name().c_str());
            z->no_corpse_quiet = true;
            z->no_extra_death_drops = true;
            z->die(nullptr);
            return false;
        }
    }

    if( z->friendly ) {
        // Friendly (hacked?) bot ignore the player.
        // TODO: might need to be revisited when it can target npcs.
        return false;
    }
    z->moves -= 150;
    add_msg(m_warning, _("The %s takes your picture!"), z->name().c_str());
    // TODO: Make the player known to the faction
    g->add_event(EVENT_ROBOT_ATTACK, int(calendar::turn) + rng(15, 30), 0,
                 g->u.global_sm_location());

    return true;
}

bool mattack::tazer(monster *z)
{
    Creature *target = z->attack_target();
    if( target == nullptr || !is_adjacent( z, target, false ) ) {
        return false;
    }

    taze( z, target );
    return true;
}

void mattack::taze( monster *z, Creature *target )
{
    z->moves -= 200;   // It takes a while
    if( target == nullptr || target->uncanny_dodge() ) {
        return;
    }

    int dam = target->deal_damage( z, bp_torso, damage_instance( DT_ELECTRIC, rng( 1, 5 ) ) ).total_damage();
    if( dam == 0 ) {
        target->add_msg_player_or_npc( _("The %s unsuccessfully attempts to shock you."),
                                       _("The %s unsuccessfully attempts to shock <npcname>."),
                                       z->name().c_str() );
        return;
    }

    auto m_type = target->attitude_to( g->u ) == Creature::A_FRIENDLY ? m_bad : m_neutral;
    target->add_msg_player_or_npc( m_type,
                                   _("The %s shocks you!"),
                                   _("The %s shocks <npcname>!"),
                                   z->name().c_str() );
    target->check_dead_state();
}

void mattack::rifle( monster *z, Creature *target )
{
    const std::string ammo_type("556");
    // Make sure our ammo isn't weird.
    if (z->ammo[ammo_type] > 2000) {
        debugmsg("Generated too much ammo (%d) for %s in mattack::rifle", z->ammo[ammo_type], z->name().c_str());
        z->ammo[ammo_type] = 2000;
    }

    npc tmp = make_fake_npc(z, 16, 10, 8, 12);
    tmp.set_skill_level( skill_rifle, 8 );
    tmp.set_skill_level( skill_gun, 6 );
    tmp.recoil = 0; // no need to aim

    if( target == &g->u ) {
        if (!z->has_effect( effect_targeted )) {
            sounds::sound(z->pos(), 8, _("beep-beep."));
            z->add_effect( effect_targeted, 8);
            z->moves -= 100;
            return;
        }
    }
    z->moves -= 150;   // It takes a while

    if (z->ammo[ammo_type] <= 0) {
        if (one_in(3)) {
            sounds::sound(z->pos(), 2, _("a chk!"));
        } else if (one_in(4)) {
            sounds::sound(z->pos(), 6, _("boop!"));
        }
        return;
    }
    if (g->u.sees( *z )) {
        add_msg(m_warning, _("The %s opens up with its rifle!"), z->name().c_str());
    }

    tmp.weapon = item( "m4a1" ).ammo_set( ammo_type, z->ammo[ ammo_type ] );
    int burst = std::max( tmp.weapon.gun_get_mode( "AUTO" ).qty, 1 );

    z->ammo[ ammo_type ] -= tmp.fire_gun( target->pos(), burst ) * tmp.weapon.ammo_required();

    if (target == &g->u) {
        z->add_effect( effect_targeted, 3);
    }
}

void mattack::frag( monster *z, Creature *target ) // This is for the bots, not a standalone turret
{
    const std::string ammo_type("40mm_frag");
    // Make sure our ammo isn't weird.
    if (z->ammo[ammo_type] > 100) {
        debugmsg("Generated too much ammo (%d) for %s in mattack::frag", z->ammo[ammo_type], z->name().c_str());
        z->ammo[ammo_type] = 100;
    }

    if( target == &g->u ) {
        if (!z->has_effect( effect_targeted )) {
            //~Potential grenading detected.
            add_msg(m_warning, _("Those laser dots don't seem very friendly...") );
            g->u.add_effect( effect_laserlocked, 3); // Effect removed in game.cpp, duration doesn't much matter
            sounds::sound(z->pos(), 10, _("Targeting."));
            z->add_effect( effect_targeted, 5);
            z->moves -= 150;
            // Should give some ability to get behind cover,
            // even though it's patently unrealistic.
            return;
        }
    }
    npc tmp = make_fake_npc(z, 16, 10, 8, 12);
    tmp.set_skill_level( skill_launcher, 8 );
    tmp.set_skill_level( skill_gun, 6 );
    tmp.recoil = 0; // no need to aim
    z->moves -= 150;   // It takes a while

    if (z->ammo[ammo_type] <= 0) {
        if (one_in(3)) {
            sounds::sound(z->pos(), 2, _("a chk!"));
        } else if (one_in(4)) {
            sounds::sound(z->pos(), 6, _("boop!"));
        }
        return;
    }
    if (g->u.sees( *z )) {
        add_msg(m_warning, _("The %s's grenade launcher fires!"), z->name().c_str());
    }

    tmp.weapon = item( "mgl" ).ammo_set( ammo_type, z->ammo[ ammo_type ] );
    int burst = std::max( tmp.weapon.gun_get_mode( "AUTO" ).qty, 1 );

    z->ammo[ ammo_type ] -= tmp.fire_gun( target->pos(), burst ) * tmp.weapon.ammo_required();

    if (target == &g->u) {
        z->add_effect( effect_targeted, 3);
    }
}

void mattack::tankgun( monster *z, Creature *target )
{
    const std::string ammo_type("120mm_HEAT");
    // Make sure our ammo isn't weird.
    if (z->ammo[ammo_type] > 40) {
        debugmsg("Generated too much ammo (%d) for %s in mattack::tankgun", z->ammo[ammo_type], z->name().c_str());
        z->ammo[ammo_type] = 40;
    }

    tripoint aim_point;
    int dist = rl_dist( z->pos(), target->pos() );
    aim_point = target->pos();
    if( dist > 50 ) {
        return;
    }

    if (!z->has_effect( effect_targeted )) {
        //~ There will be a 120mm HEAT shell sent at high speed to your location next turn.
        target->add_msg_if_player( m_warning, _("You're not sure why you've got a laser dot on you...") );
        //~ Sound of a tank turret swiveling into place
        sounds::sound(z->pos(), 10, _("whirrrrrclick."));
        z->add_effect( effect_targeted, 10 );
        target->add_effect( effect_laserlocked, 10 );
        z->moves -= 200;
        // Should give some ability to get behind cover,
        // even though it's patently unrealistic.
        return;
    }
    // Target the vehicle itself instead if there is one.
    vehicle *veh = g->m.veh_at( target->pos() );
    if( veh != nullptr ) {
        aim_point = veh->global_pos3() + veh->rotated_center_of_mass();
    }
    // kevingranade KA101: yes, but make it really inaccurate
    // Sure thing.
    npc tmp = make_fake_npc(z, 12, 8, 8, 8);
    tmp.set_skill_level( skill_launcher, 1 );
    tmp.set_skill_level( skill_gun, 1 );
    tmp.recoil = 0; // no need to aim
    z->moves -= 150;   // It takes a while

    if (z->ammo[ammo_type] <= 0) {
        if (one_in(3)) {
            sounds::sound(z->pos(), 2, _("a chk!"));
        } else if (one_in(4)) {
            sounds::sound(z->pos(), 6, _("clank!"));
        }
        return;
    }
    if (g->u.sees( *z )) {
        add_msg(m_warning, _("The %s's 120mm cannon fires!"), z->name().c_str());
    }
    tmp.weapon = item( "TANK" ).ammo_set( ammo_type, z->ammo[ ammo_type ] );
    int burst = std::max( tmp.weapon.gun_get_mode( "AUTO" ).qty, 1 );

    z->ammo[ ammo_type ] -= tmp.fire_gun( target->pos(), burst ) * tmp.weapon.ammo_required();
}

bool mattack::searchlight(monster *z)
{

    int max_lamp_count = 3;
    if (z->get_hp() < z->get_hp_max()) {
        max_lamp_count--;
    }
    if (z->get_hp() < z->get_hp_max() / 3) {
        max_lamp_count--;
    }

    const int zposx = z->posx();
    const int zposy = z->posy();

    //this searchlight is not initialized
    if (z->inv.size() == 0) {

        for (int i = 0; i < max_lamp_count; i++) {

            item settings("processor", 0);

            settings.set_var( "SL_PREFER_UP", "TRUE" );
            settings.set_var( "SL_PREFER_DOWN", "TRUE" );
            settings.set_var( "SL_PREFER_RIGHT", "TRUE" );
            settings.set_var( "SL_PREFER_LEFT", "TRUE" );

            for (int x = zposx - 24; x < zposx + 24; x++)
                for (int y = zposy - 24; y < zposy + 24; y++) {
                    tripoint dest( x, y, z->posz() );
                    if (g->mon_at( dest ) != -1 && g->zombie(g->mon_at( dest )).type->id == mon_turret_searchlight) {
                        if (x < zposx) {
                            settings.set_var( "SL_PREFER_LEFT", "FALSE" );
                        }
                        if (x > zposx) {
                            settings.set_var( "SL_PREFER_RIGHT", "FALSE" );
                        }
                        if (y < zposy) {
                            settings.set_var( "SL_PREFER_UP", "FALSE" );
                        }
                        if (y > zposy) {
                            settings.set_var( "SL_PREFER_DOWN", "FALSE" );
                        }
                    }

                }

            settings.set_var( "SL_SPOT_X", 0 );
            settings.set_var( "SL_SPOT_Y", 0 );

            z->add_item(settings);
        }
    }

    //battery charge from the generator is enough for some time of work
    if( calendar::once_every(MINUTES(10)) ) {

        bool generator_ok = false;

        for (int x = zposx - 24; x < zposx + 24; x++) {
            for (int y = zposy - 24; y < zposy + 24; y++) {
                tripoint dest( x, y, z->posz() );
                if( g->m.ter( dest ) == ter_str_id( "t_plut_generator" ) ) {
                    generator_ok = true;
                }
            }
        }

        if (!generator_ok) {
            for ( auto &settings: z->inv ) {
                settings.set_var( "SL_POWER", "OFF" );
            }

            return true;
        }
    }

    for (int i = 0; i < max_lamp_count; i++) {

        item &settings = z->inv[i];

        if (settings.get_var( "SL_POWER" )  == "OFF") {
            return true;
        }

        const int rng_dir = rng(0, 7);

        if (one_in(5)) {

            if (!one_in(5)) {
                settings.set_var( "SL_DIR", rng_dir );
            } else {
                const int rng_pref = rng(0, 3) * 2;
                if (rng_pref == 0 && settings.get_var( "SL_PREFER_UP" ) == "TRUE") {
                    settings.set_var( "SL_DIR", rng_pref );
                } else            if (rng_pref == 2 && settings.get_var( "SL_PREFER_RIGHT" ) == "TRUE") {
                    settings.set_var( "SL_DIR", rng_pref );
                } else            if (rng_pref == 4 && settings.get_var( "SL_PREFER_DOWN" ) == "TRUE") {
                    settings.set_var( "SL_DIR", rng_pref );
                } else            if (rng_pref == 6 && settings.get_var( "SL_PREFER_LEFT" ) == "TRUE") {
                    settings.set_var( "SL_DIR", rng_pref );
                }
            }
        }


        int x = zposx + settings.get_var( "SL_SPOT_X", 0 );
        int y = zposy + settings.get_var( "SL_SPOT_Y", 0 );
        int shift = 0;

        for (int i = 0; i < rng(1, 2); i++) {

            if (!z->sees( g->u )) {
                shift = settings.get_var( "SL_DIR", shift );

                switch (shift) {
                    case 0:
                        y--;
                        break;
                    case 1:
                        y--;
                        x++;
                        break;
                    case 2:
                        x++;
                        break;
                    case 3:
                        x++;
                        y++;
                        break;
                    case 4:
                        y++;
                        break;
                    case 5:
                        y++;
                        x--;
                        break;
                    case 6:
                        x--;
                        break;
                    case 7:
                        x--;
                        y--;
                        break;

                    default:
                        break;
                }

            } else {
                if (x < g->u.posx()) {
                    x++;
                }
                if (x > g->u.posx()) {
                    x--;
                }
                if (y < g->u.posy()) {
                    y++;
                }
                if (y > g->u.posy()) {
                    y--;
                }
            }

            if (rl_dist(x, y, zposx, zposy) > 50) {
                if (x > zposx) {
                    x--;
                }
                if (x < zposx) {
                    x++;
                }
                if (y > zposy) {
                    y--;
                }
                if (y < zposy) {
                    y++;
                }
            }
        }

        settings.set_var( "SL_SPOT_X", x - zposx );
        settings.set_var( "SL_SPOT_Y", y - zposy );

        g->m.add_field( tripoint( x, y, z->posz() ), fd_spotlight, 1, 0 );

    }

    return true;
}

bool mattack::flamethrower(monster *z)
{
    if( z->friendly ) {
        return false; // TODO: handle friendly monsters
    }
    if (z->friendly != 0) {
        Creature *target = nullptr;

        // Attacking monsters, not the player!
        int boo_hoo;
        target = z->auto_find_hostile_target( 5, boo_hoo );
        if (target == NULL) {// Couldn't find any targets!
            if(boo_hoo > 0 && g->u.sees( *z ) ) { // because that stupid oaf was in the way!
                add_msg(m_warning, ngettext("Pointed in your direction, the %s emits an IFF warning beep.",
                                            "Pointed in your direction, the %s emits %d annoyed sounding beeps.",
                                            boo_hoo),
                        z->name().c_str(), boo_hoo);
            }
            return false; // did reset before refactor, changed to match other turret behaviors
        }
        flame( z, target );
        return true;
    }

    int dist = within_visual_range(z, 5);
    if (dist < 0) {
        return false;
    }

    flame( z, &g->u );

    return true;
}

void mattack::flame( monster *z, Creature *target )
{
    int dist = rl_dist( z->pos(), target->pos() );
    if( target != &g->u ) {
        // friendly
        z->moves -= 500;   // It takes a while
        if( !g->m.sees( z->pos(), target->pos(), dist ) ) {
            // shouldn't happen
            debugmsg( "mattack::flame invoked on invisible target" );
        }
        std::vector<tripoint> traj = g->m.find_clear_path( z->pos(), target->pos() );

        for (auto &i : traj) {
            // break out of attack if flame hits a wall
            // TODO: Z
            if (g->m.hit_with_fire( tripoint( i.x, i.y, z->posz() ) )) {
                if (g->u.sees( i ))
                    add_msg(_("The tongue of flame hits the %s!"),
                            g->m.tername(i.x, i.y).c_str());
                return;
            }
            g->m.add_field( i, fd_fire, 1, 0 );
        }
        target->add_effect( effect_onfire, 8, bp_torso );

        return;
    }

    z->moves -= 500;   // It takes a while
    if( !g->m.sees( z->pos(), target->pos(), dist + 1 ) ) {
        // shouldn't happen
        debugmsg( "mattack::flame invoked on invisible target" );
    }
    std::vector<tripoint> traj = g->m.find_clear_path( z->pos(), target->pos() );

    for (auto &i : traj) {
        // break out of attack if flame hits a wall
        if (g->m.hit_with_fire( tripoint( i.x, i.y, z->posz() ) )) {
            if (g->u.sees( i ))
                add_msg(_("The tongue of flame hits the %s!"),
                        g->m.tername(i.x, i.y).c_str());
            return;
        }
        g->m.add_field(i, fd_fire, 1, 0);
    }
    if( !target->uncanny_dodge() ) {
        target->add_effect( effect_onfire, 8, bp_torso );
    }
}

bool mattack::copbot(monster *z)
{
    Creature *target = z->attack_target();
    if( target == nullptr ) {
        return false;
    }

    // TODO: Make it recognize zeds as human, but ignore animals
    player *foe = dynamic_cast<player*>( target );
    bool sees_u = foe != nullptr && z->sees( *foe );
    bool cuffed = foe != nullptr && foe->weapon.typeId() == "e_handcuffs";
    // Taze first, then ask questions (simplifies later checks for non-humans)
    if( !cuffed && is_adjacent( z, target, true ) ) {
        taze( z, target );
        return true;
    }

    if( rl_dist( z->pos(), target->pos() ) > 2 || foe == nullptr || !z->sees( *target ) ) {
        if (one_in(3)) {
            if (sees_u) {
                if ( foe != nullptr && foe->unarmed_attack() ) {
                    sounds::sound(z->pos(), 18, _("a robotic voice boom, \"Citizen, Halt!\""));
                } else if( foe != nullptr && !cuffed ) {
                    sounds::sound(z->pos(), 18, _("a robotic voice boom, \"\
Please put down your weapon.\""));
                }
            } else
                sounds::sound(z->pos(), 18,
                         _("a robotic voice boom, \"Come out with your hands up!\""));
        } else {
            sounds::sound(z->pos(), 18, _("a police siren, whoop WHOOP"));
        }
        return true;
    }

    // If cuffed don't attack the player, unless the bot is damaged
    // presumably because of the player's actions
    if( z->get_hp() == z->get_hp_max() ) {
        z->anger = 1;
    } else {
        z->anger = z->type->agro;
    }

    return true;
}

bool mattack::chickenbot(monster *z)
{
    int mode = 0;
    int boo_hoo = 0;
    Creature *target;
    if( z->friendly == 0 ) {
        target = z->attack_target();
        if( target == nullptr ) {
            return false;
        }
    } else {
        target = z->auto_find_hostile_target( 38, boo_hoo );
        if( target == nullptr ) {
            if( boo_hoo > 0 && g->u.sees( *z ) ) { // because that stupid oaf was in the way!
                add_msg(m_warning, ngettext("Pointed in your direction, the %s emits an IFF warning beep.",
                                            "Pointed in your direction, the %s emits %d annoyed sounding beeps.",
                                            boo_hoo),
                        z->name().c_str(), boo_hoo);
            }
            return false;
        }
    }

    int cap = target->power_rating() - 1;
    monster *mon = dynamic_cast< monster* >( target );
    // Their attitude to us and not ours to them, so that bobcats won't get gunned down
    // Only monster-types for now - assuming humans are smart enough not to make it obvious
    // Unless damaged - then everything is hostile
    if( z->get_hp() <= z->get_hp_max() ||
        ( mon != nullptr && mon->attitude_to( *z ) == Creature::Attitude::A_HOSTILE ) ) {
        cap += 2;
    }

    int dist = rl_dist( z->pos(), target->pos() );
    if( dist == 1 && one_in(2) ) {
        mode = 1;
    } else if( ( dist >= 12) ||
               ( ( z->friendly != 0 || g->u.in_vehicle ) && dist >= 6 ) ||
               cap > 2 ) {
        mode = 3;
    } else if( dist >= 4) {
        mode = 2;
    }

    if (mode == 0) {
        return false;    // No attacks were valid!
    }

    if( mode > cap ) {
        mode = cap;
    }
    switch (mode) {
    case 0:
    case 1:
        if( dist <= 1 ) {
            taze( z, target );
        }
        break;
    case 2:
        if( dist <= 20 ) {
            rifle( z, target );
        }
        break;
    case 3:
        if( dist == 38 ) {
            frag( z, target );
        }
        break;
    default:
        return false; // Weak stuff, shouldn't bother with
    }

    return true;
}

bool mattack::multi_robot(monster *z)
{
    int mode = 0;
    int boo_hoo = 0;
    Creature *target;
    if( z->friendly == 0 ) {
        target = z->attack_target();
        if( target == nullptr ) {
            return false;
        }
    } else {
        target = z->auto_find_hostile_target( 48, boo_hoo );
        if( target == nullptr ) {
            if( boo_hoo > 0 && g->u.sees( *z ) ) { // because that stupid oaf was in the way!
                add_msg(m_warning, ngettext("Pointed in your direction, the %s emits an IFF warning beep.",
                                            "Pointed in your direction, the %s emits %d annoyed sounding beeps.",
                                            boo_hoo),
                        z->name().c_str(), boo_hoo);
            }
            return false;
        }
    }

    int cap = target->power_rating();
    monster *mon = dynamic_cast< monster* >( target );
    // Their attitude to us and not ours to them, so that bobcats won't get gunned down
    // Only monster-types for now - assuming humans are smart enough not to make it obvious
    // Unless damaged - then everything is hostile
    if( z->get_hp() <= z->get_hp_max() ||
        ( mon != nullptr && mon->attitude_to( *z ) == Creature::Attitude::A_HOSTILE ) ) {
        cap += 2;
    }

    int dist = rl_dist( z->pos(), target->pos() );
    if( dist == 1 && one_in(2) ) {
        mode = 1;
    } else if( dist <= 5 ) {
        mode = 2;
    } else if( dist <= 20 ) {
        mode = 3;
    } else if( dist <= 30 ) {
        mode = 4;
    } else if( ( target == &g->u && g->u.in_vehicle ) ||
                 z->friendly != 0 ||
                 cap > 4 ) {
        // Primary only kicks in if you're in a vehicle or are big enough to be mistaken for one.
        // Or if you've hacked it so the turret's on your side.  ;-)
        if( dist >= 35 && dist < 50 ) {
            // Enforced max-range of 50.
            mode = 5;
            cap = 5;
        }
    }

    if( mode == 0 ) {
        return false;    // No attacks were valid!
    }

    if( mode > cap ) {
        mode = cap;
    }
    switch (mode) {
    case 1:
        if( dist <= 1 ) {
            taze( z, target );
        }
        break;
    case 2:
        if( dist <= 5 ) {
            flame( z, target );
        }
        break;
    case 3:
        if( dist <= 20 ) {
            rifle( z, target );
        }
        break;
    case 4:
        if( dist <= 30 ) {
            frag( z, target );
        }
        break;
    case 5:
        if( dist <= 50 ) {
            tankgun( z, target );
        }
        break;
    default:
        return false; // Weak stuff, shouldn't bother with
    }

    return true;
}

bool mattack::ratking(monster *z)
{
    if( z->friendly ) {
        return false; // TODO: handle friendly monsters
    }
    // Disable z-level ratting or it can get silly
    if( rl_dist( z->pos(), g->u.pos() ) > 50 || z->posz() != g->u.posz() ) {
        return false;
    }

    switch (rng(1, 5)) { // What do we say?
    case 1:
        add_msg(m_warning, _("\"YOU... ARE FILTH...\""));
        break;
    case 2:
        add_msg(m_warning, _("\"VERMIN... YOU ARE VERMIN...\""));
        break;
    case 3:
        add_msg(m_warning, _("\"LEAVE NOW...\""));
        break;
    case 4:
        add_msg(m_warning, _("\"WE... WILL FEAST... UPON YOU...\""));
        break;
    case 5:
        add_msg(m_warning, _("\"FOUL INTERLOPER...\""));
        break;
    }
    if (rl_dist( z->pos(), g->u.pos() ) <= 10) {
        g->u.add_effect( effect_rat, 30);
    }

    return true;
}

bool mattack::generator(monster *z)
{
    sounds::sound(z->pos(), 100, "");
    if( calendar::once_every(MINUTES(1)) && z->get_hp() < z->get_hp_max() ) {
        z->heal( 1 );
    }

    return true;
}

bool mattack::upgrade(monster *z)
{
    std::vector<int> targets;
    for (size_t i = 0; i < g->num_zombies(); i++) {
        monster &zed = g->zombie(i);
        // Check this first because it is a relatively cheap check
        if( zed.can_upgrade()) {
            // Then do the more expensive ones
            if ( z->attitude_to( zed ) != Creature::Attitude::A_HOSTILE &&
                 within_target_range(z, &zed, 10) ) {
                targets.push_back(i);
            }
        }
    }
    if (targets.empty()) {
        // Nobody to upgrade, get MAD!
        z->anger = 100;
        return false;
    } else {
        // We've got zombies to upgrade now, calm down again
        z->anger = 5;
    }

    z->moves -= z->type->speed; // Takes one turn

    monster *target = &( g->zombie( random_entry( targets ) ) );

    std::string old_name = target->name();
    const auto could_see = g->u.sees( *target );
    target->hasten_upgrade();
    target->try_upgrade(false);
    const auto can_see = g->u.sees( *target );
    if (g->u.sees( *z )) {
        if (could_see) {
            //~ %1$s is the name of the zombie upgrading the other, %2$s is the zombie being upgraded.
            add_msg(m_warning, _("A black mist floats from the %1$s around the %2$s."),
                     z->name().c_str(), old_name.c_str());
        } else {
            add_msg(m_warning, _("A black mist floats from the %s."), z->name().c_str());
        }
    }
    if (target->name() != old_name) {
        if( could_see && can_see ) {
            //~ %1$s is the pre-upgrade monster, %2$s is the post-upgrade monster.
            add_msg(m_warning, _("The %1$s becomes a %2$s!"), old_name.c_str(),
                     target->name().c_str());
        } else if( could_see ) {
            add_msg( m_warning, _( "The %s vanishes!" ), old_name.c_str() );
        } else if( can_see ) {
            add_msg( m_warning, _( "A %s appears!"), target->name().c_str() );
        }
    }

    return true;
}

bool mattack::breathe(monster *z)
{
    z->moves -= 100;   // It takes a while

    bool able = (z->type->id == mon_breather_hub);
    if( !able ) {
        for( const tripoint &dest : g->m.points_in_radius( z->pos(), 3 ) ) {
            int mondex = g->mon_at(dest);
            if( mondex != -1 && g->zombie(mondex).type->id == mon_breather_hub ) {
                able = true;
                break;
            }
        }
    }
    if( !able ) {
        return true;
    }

    std::vector<tripoint> valid;
    for( const tripoint &dest : g->m.points_in_radius( z->pos(), 1 ) ) {
        if( g->is_empty( dest ) ) {
            valid.push_back( dest );
        }
    }

    if( !valid.empty() ) {
        const tripoint pt = random_entry( valid );
        if (g->summon_mon(mon_breather, pt)) {
            monster *spawned = g->monster_at(pt);
            spawned->reset_special("BREATHE");
            spawned->make_ally(z);
        }
    }

    return true;
}

bool mattack::stretch_bite(monster *z)
{
    if( !z->can_act() ) {
        return false;
    }

    // Let it be used on non-player creatures
    // can be used at close range too!
    Creature *target = z->attack_target();
    if( target == nullptr || rl_dist( z->pos(), target->pos() ) > 3 || !z->sees(*target)) {
        return false;
    }

    z->moves -= 150;

    for (auto &pnt : g->m.find_clear_path( z->pos(), target->pos() ) ){
        if( g->m.impassable( pnt ) ){
            z->add_effect( effect_stunned, 6);
            target->add_msg_player_or_npc( _("The %1$s stretches its head at you, but bounces off the %2$s"),
                                           _("The %1$s stretches its head at <npcname>, but bounces off the %2$s"),
                                           z->name().c_str(), g->m.obstacle_name( pnt ).c_str() );
            return true;
        }
    }
    bool uncanny = target->uncanny_dodge();
    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if( uncanny || dodge_check(z, target) ) {
        z->moves -= 150;
        z->add_effect( effect_stunned, 3);
        auto msg_type = target == &g->u ? m_warning : m_info;
        target->add_msg_player_or_npc( msg_type, _("The %s's head extends to bite you, but you dodge and the head sails past!"),
                                              _("The %s's head extends to bite <npcname>, but they dodge and the head sails past!"),
                                    z->name().c_str() );
        if( !uncanny ) {
            target->on_dodge( z, z->type->melee_skill * 2 );
        }
        return true;
    }

    body_part hit = target->get_random_body_part();
    int dam = rng(5, 15); //more damage due to the speed of the moving head
    dam = target->deal_damage( z, hit, damage_instance( DT_STAB, dam ) ).total_damage();

    if( dam > 0 ) {
        auto msg_type = target == &g->u ? m_bad : m_info;
        //~ 1$s is monster name, 2$s bodypart in accusative
        target->add_msg_player_or_npc( msg_type,
                                    _("The %1$s's teeth sink into your %2$s!"),
                                    _("The %1$s's teeth sink into <npcname>'s %2$s!"),
                                    z->name().c_str(),
                                    body_part_name_accusative( hit ).c_str() );

        if( one_in( 16 - dam ) ) {
            if( target->has_effect( effect_bite, hit)) {
                target->add_effect( effect_bite, 400, hit, true);
            } else if( target->has_effect( effect_infected, hit ) ) {
                target->add_effect( effect_infected, 250, hit, true );
            } else {
                target->add_effect( effect_bite, 1, hit, true );
            }
        }
    } else {
        target->add_msg_player_or_npc( _("The %1$s's head hits your %2$s, but glances off your armor!"),
                                    _("The %1$s's head hits <npcname>'s %2$s, but glances off armor!"),
                                    z->name().c_str(),
                                    body_part_name_accusative( hit ).c_str() );
    }

    target->on_hit( z, hit,  z->type->melee_skill );

    return true;
}

bool mattack::brandish(monster *z)
{
    if( z->friendly ) {
        return false; // TODO: handle friendly monsters
    }
    if( !z->sees( g->u ) ) {
        return false; // Only brandish if we can see you!
    }
    add_msg(m_warning, _("He's brandishing a knife!"));
    add_msg(_("Quiet, quiet"));

    return true;
}

bool mattack::flesh_golem(monster *z)
{
    if( !z->can_act() ) {
        return false;
    }

    Creature *target = z->attack_target();
    if( target == nullptr ) {
        return false;
    }

    int dist = rl_dist( z->pos(), target->pos() );
    if( dist > 20 ||
        !z->sees( *target ) ) {
        return false;
    }

    if( dist > 1 ) {
        if( one_in(12) ) {
            z->moves -= 200;
            // It doesn't "nearly deafen you" when it roars from the other side of bubble
            sounds::sound( z->pos(), 80, _("a terrifying roar!") );
            return true;
        }
        return false;
    }
    if( !is_adjacent( z, target, true ) ) {
        // No attacking through floor, even if we can see the target somehow
        return false;
    }
    if( g->u.sees( *z ) ) {
        add_msg(_("The %1$s swings a massive claw at %2$s!"), z->name().c_str(), target->disp_name().c_str() );
    }
    z->moves -= 100;

    if( target->uncanny_dodge() ) {
        return true;
    }

    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if (dodge_check(z, target)) {
        target->add_msg_player_or_npc( _("You dodge it!"),
                                       _("<npcname> dodges it!") );
        target->on_dodge( z, z->type->melee_skill * 2 );
        return true;
    }
    body_part hit = target->get_random_body_part();
    // TODO: 10 bashing damage doesn't sound like a "massive claw" but a mediocre punch
    int dam = rng(5, 10);
    //~ 1$s is bodypart name, 2$d is damage value.
    target->deal_damage( z, hit, damage_instance( DT_BASH, dam ) );
    if( one_in( 6 ) ) {
        target->add_effect( effect_downed, 30);
    }

    target->add_msg_if_player(m_bad, _("Your %1$s is battered for %2$d damage!"), body_part_name(hit).c_str(), dam);
    target->on_hit( z, hit,  z->type->melee_skill );

    return true;
}

bool mattack::lunge(monster *z)
{
    if( !z->can_act() ) {
        return false;
    }

    Creature *target = z->attack_target();
    if( target == nullptr ) {
        return false;
    }

    int dist = rl_dist( z->pos(), target->pos() );
    if( dist > 20 ||
        !z->sees( *target ) ) {
        return false;
    }

    bool seen = g->u.sees( *z );
    if( dist > 1 ) {
        if (one_in(5)) {
            if( dist > 4 || !z->sees( *target ) ) {
                return false; // Out of range
            }
            z->moves += 200;
            if( seen ) {
                add_msg(_("The %1$s lunges for %2$s!"), z->name().c_str(), target->disp_name().c_str() );
            }
            return true;
        }
        return false;
    }

    if( !is_adjacent( z, target, false ) ) {
        // No attacking up or down - lunging requires contact
        // There could be a lunge down attack, though
        return false;
    }

    z->moves -= 100;

    if( target->uncanny_dodge()) {
        return true;
    }

    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if (dodge_check(z, target)) {
        target->add_msg_player_or_npc( _("The %1$s lunges at you, but you sidestep it!"),
                                       _("The %1$s lunges at <npcname>, but they sidestep it!"), z->name().c_str() );
        target->on_dodge( z, z->type->melee_skill * 2 );
        return true;
    }
    body_part hit = target->get_random_body_part();
    int dam = rng(3, 7);
    dam = target->deal_damage( z, hit, damage_instance( DT_BASH, dam ) ).total_damage();
    if( dam > 0 ) {
        auto msg_type = target == &g->u ? m_bad : m_warning;
        target->add_msg_player_or_npc( msg_type, _("The %1$s lunges at your %2$s, battering it for %3$d damage!"),
                                    _("The %1$s lunges at <npcname>'s %2$s, battering it for %3$d damage!"),
                                    z->name().c_str(), body_part_name(hit).c_str(), dam);
    } else {
        target->add_msg_player_or_npc( _("The %1$s lunges at your %2$s, but your armor prevents injury!"),
                                    _("The %1$s lunges at <npcname>'s %2$s, but their armor prevents injury!"),
                                    z->name().c_str(),
                                    body_part_name_accusative( hit ).c_str() );
    }
    if( one_in( 6 ) ) {
        target->add_effect( effect_downed, 3);
    }
    target->on_hit( z, hit,  z->type->melee_skill );
    target->check_dead_state();
    return true;
}

bool mattack::longswipe(monster *z)
{
    if( z->friendly ) {
        return false; // TODO: handle friendly monsters
    }
    Creature *target = z->attack_target();
    if( target == nullptr ) {
        return false;
    }
    if( rl_dist( z->pos(), target->pos() ) > 3 || !z->sees( *target ) ) {
        return false; //out of range
    }
    //Is there something impassable blocking the claw?
    for( const auto &pnt : g->m.find_clear_path( z->pos(), target->pos() ) ){
        if( g->m.impassable(pnt) ) {
            //If we're here, it's an unadjacent attack, which is only attempted 1/5 of the time.
            if( !one_in( 5 ) ) {
                return false;
            }
            target->add_msg_player_or_npc( _( "The %1$s thrusts a claw at you, but it bounces off the %2$s!" ),
                                           _( "The %1$s thrusts a claw at <npcname>, but it bounces off the %2$s!" ),
                                           z->name().c_str(), g->m.obstacle_name( pnt ).c_str() );
            z->mod_moves( -150 );
            return true;
        }
    }

    if( !is_adjacent( z, target, true ) ) {
        if (one_in(5)) {

            z->moves -= 150;

            if (target->uncanny_dodge()) {
                return true;
            }
            // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
            if (dodge_check(z, target)) {
                target->add_msg_player_or_npc( _("The %s thrusts a claw at you, but you evade it!"),
                                            _("The %s thrusts a claw at <npcname>, but they evade it!"),
                                            z->name().c_str() );
                target->on_dodge( z, z->type->melee_skill * 2  );
                return true;
            }
            body_part hit = target->get_random_body_part();
            int dam = rng(3, 7);
            dam = target->deal_damage( z, hit, damage_instance( DT_CUT, dam ) ).total_damage();
            if( dam > 0 ) {
                auto msg_type = target == &g->u ? m_bad : m_warning;
                //~ 1$s is bodypart name, 2$d is damage value.
                target->add_msg_player_or_npc( msg_type, _("The %1$s thrusts a claw at your %2$s, slashing it for %3$d damage!"),
                                            _("The %1$s thrusts a claw at <npcname>'s %2$s, slashing it for %3$d damage!"),
                                            z->name().c_str(), body_part_name(hit).c_str(), dam);
            } else {
                target->add_msg_player_or_npc( _("The %1$s thrusts a claw at your %2$s, but glances off your armor!"),
                                    _("The %1$s thrusts a claw at <npcname>'s %2$s, but glances off armor!"),
                                    z->name().c_str(),
                                    body_part_name_accusative( hit ).c_str() );
            }
            target->on_hit( z, hit,  z->type->melee_skill  );
            return true;
        }
        return false;
    }
    z->moves -= 100;

    if (target->uncanny_dodge()) {
        return true;
    }

    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if (dodge_check(z, target)) {
        target->add_msg_player_or_npc( _("The %s slashes at your neck! You duck!"),
                                    _("The %s slashes at <npcname>'s neck! They duck!"), z->name().c_str() );
        target->on_dodge( z, z->type->melee_skill * 2  );
        return true;
    }
    body_part hit = bp_head;
    int dam = rng(6, 10);
    dam = target->deal_damage( z, hit, damage_instance( DT_CUT, dam ) ).total_damage();
    if( dam > 0 ) {
        auto msg_type = target == &g->u ? m_bad : m_warning;
        target->add_msg_player_or_npc(msg_type, _("The %1$s slashes at your neck, cutting your throat for %2$d damage!"),
                                                _("The %1$s slashes at <npcname>'s neck, cutting their throat for %2$d damage!"),
                                                z->name().c_str(), dam );
        target->add_effect( effect_bleed, 100, hit);
    } else {
        target->add_msg_player_or_npc( _("The %1$s slashes at your %2$s, but glances off your armor!"),
                                    _("The %1$s slashes at <npcname>'s %2$s, but glances off armor!"),
                                    z->name().c_str(),
                                    body_part_name_accusative( hit ).c_str() );
    }
    target->on_hit( z, hit,  z->type->melee_skill  );
    target->check_dead_state();

    return true;
}


bool mattack::parrot(monster *z)
{
    if( z->has_effect( effect_shrieking ) ) {
        sounds::sound(z->pos(), 120, _("a piercing wail!"), true);
        z->moves -= 40;
        return false;
    } else if (one_in(20)) {
        z->moves -= 100;  // It takes a while
        const SpeechBubble speech = get_speech( z->type->id.str() );
        sounds::sound(z->pos(), speech.volume, speech.text);
        return true;
    }

    return false;
}

bool mattack::darkman(monster *z)
{
    if( z->friendly ) {
        return false; // TODO: handle friendly monsters
    }
    if( rl_dist( z->pos(), g->u.pos() ) > 40 ) {
        return false;
    }
    std::vector<tripoint> free;
    for( const tripoint &dest : g->m.points_in_radius( z->pos(), 1 ) ) {
        if( g->is_empty(dest) ) {
            free.push_back(dest);
        }
    }
    if (!free.empty()) {
        z->moves -= 10;
        const tripoint target = random_entry( free );
        if (g->summon_mon(mon_shadow, target)) {
            monster *shadow = g->monster_at( target );
            shadow->make_ally(z);
        }
        if( g->u.sees( *z ) ) {
            add_msg(m_warning, _("A shadow splits from the %s!"),
                    z->name().c_str() );
        }
    }
    if( !z->sees( g->u ) ) {
        return true; // Wont do the combat stuff unless it can see you
    }
    switch (rng(1, 7)) { // What do we say?
    case 1:
        add_msg(_("\"Stop it please\""));
        break;
    case 2:
        add_msg(_("\"Let us help you\""));
        break;
    case 3:
        add_msg(_("\"We wish you no harm\""));
        break;
    case 4:
        add_msg(_("\"Do not fear\""));
        break;
    case 5:
        add_msg(_("\"We can help you\""));
        break;
    case 6:
        add_msg(_("\"We are friendly\""));
        break;
    case 7:
        add_msg(_("\"Please dont\""));
        break;
    }
    g->u.add_effect( effect_darkness, 1, num_bp, true );

    return true;
}

bool mattack::slimespring(monster *z)
{
    if (rl_dist( z->pos(), g->u.pos() ) > 30) {
        return false;
    }

    // This morale buff effect could get spammy
    if( g->u.get_morale_level() <= 1 ) {
        switch (rng(1, 3)) { //~ Your slimes try to cheer you up!
        case 1:
            //~ Lowercase is intended: they're small voices.
            add_msg(m_good, _("\"hey, it's gonna be all right!\""));
            g->u.add_morale(MORALE_SUPPORT, 10, 50);
            break;
        case 2:
            //~ Lowercase is intended: they're small voices.
            add_msg(m_good, _("\"we'll get through this!\""));
            g->u.add_morale(MORALE_SUPPORT, 10, 50);
            break;
        case 3:
            //~ Lowercase is intended: they're small voices.
            add_msg(m_good, _("\"i'm here for you!\""));
            g->u.add_morale(MORALE_SUPPORT, 10, 50);
            break;
        }
    }
    if( rl_dist( z->pos(), g->u.pos() ) <= 3 && z->sees( g->u ) ) {
        if ( (g->u.has_effect( effect_bleed )) || (g->u.has_effect( effect_bite )) ) {
            //~ Lowercase is intended: they're small voices.
            add_msg(_("\"let me help!\""));
            // Yes, your slimespring(s) handle/don't all Bad Damage at the same time.
            if (g->u.has_effect( effect_bite )) {
                if (one_in(3)) {
                    g->u.remove_effect( effect_bite );
                    add_msg(m_good, _("The slime cleans you out!"));
                } else {
                    add_msg(_("The slime flows over you, but your gouges still ache."));
                }
            }
            if (g->u.has_effect( effect_bleed )) {
                if (one_in(2)) {
                    g->u.remove_effect( effect_bleed );
                    add_msg(m_good, _("The slime seals up your leaks!"));
                } else {
                    add_msg(_("The slime flows over you, but your fluids are still leaking."));
                }
            }
        }
    }

    return true;
}

bool mattack::thrown_by_judo(monster *z)
{
    Creature *target = z->attack_target();
    if( target == nullptr ||
        !is_adjacent( z, target, false ) ||
        !z->sees( *target ) ) {
        return false;
    }

    player *foe = dynamic_cast< player* >( target );
    if( foe == nullptr ) {
        // No mons for now
        return false;
    }
    // "Wimpy" Judo is about to pay off... :D
    if( foe->is_throw_immune() ) {
        // DX + Unarmed
        ///\EFFECT_DEX increases chance judo-throwing a monster

        ///\EFFECT_UNARMED increases chance of judo-throwing monster, vs their melee skill
        if ( ((foe->dex_cur + foe->get_skill_level( skill_unarmed )) > (z->type->melee_skill + rng(0, 3))) ) {
            target->add_msg_if_player( m_good, _("but you grab its arm and flip it to the ground!") );

            // most of the time, when not isolated
            if ( !one_in(4) && !target->is_elec_immune() && z->type->sp_defense == &mdefense::zapback ) {
                // If it all pans out, we're zap the player's arm as he flips the monster.
                target->add_msg_if_player(_("The flip does shock you..."));
                // Discounted electric damage for quick flip
                damage_instance shock;
                shock.add_damage(DT_ELECTRIC, rng(1, 3));
                foe->deal_damage(z, bp_arm_l, shock);
                foe->deal_damage(z, bp_arm_r, shock);
                foe->check_dead_state();
            }
            // Monster is down,
            z->add_effect( effect_downed, 5);
            // Deal moderate damage
            const auto damage = rng( 10, 20 );
            z->apply_damage( foe, bp_torso, damage );
            z->check_dead_state();
        } else {
            // Still avoids the major hit!
            target->add_msg_if_player(_("but you deftly spin out of its grasp!"));
        }
        return true;
    } else {
        return false;
    }
}

bool mattack::riotbot(monster *z)
{
    Creature *target = z->attack_target();
    if( target == nullptr ) {
        return false;
    }

    player *foe = dynamic_cast<player *>( target );

    if( calendar::once_every(MINUTES(1)) ) {
        for( const tripoint &dest : g->m.points_in_radius( z->pos(), 4 ) ) {
            if( g->m.passable( dest ) &&
                g->m.clear_path( z->pos(), dest, 3, 1, 100 ) ) {
                g->m.add_field( dest, fd_relax_gas, rng(1, 3), 0 );
            }
        }
    }

    //already arrested?
    //and yes, if the player has no hands, we are not going to arrest him.
    if( foe != nullptr &&
        ( foe->weapon.typeId() == "e_handcuffs" || !foe->has_two_arms() ) ) {
        z->anger = 0;

        if( calendar::once_every(25) ) {
            sounds::sound( z->pos(), 10,
                     _("Halt and submit to arrest, citizen! The police will be here any moment."));
        }

        return true;
    }

    if( z->anger < z->type->agro ) {
        z->anger += z->type->agro / 20;
        return true;
    }

    const int dist = rl_dist( z->pos(), target->pos() );

    //we need empty hands to arrest
    if( foe == &g->u && !foe->is_armed() ) {

        sounds::sound( z->pos(), 15, _("Please stay in place, citizen, do not make any movements!") );

        //we need to come closer and arrest
        if( !is_adjacent( z, foe, false ) ) {
            return true;
        }

        //Strain the atmosphere, forcing the player to wait. Let him feel the power of law!
        if( !one_in( 10 ) ) {
            foe->add_msg_player_or_npc( _("The robot carefully scans you."),
                _("The robot carefully scans <npcname>.") );
            return true;
        }

        enum {ur_arrest, ur_resist, ur_trick};

        //arrest!
        uimenu amenu;
        amenu.selected = 0;
        amenu.text = _("The riotbot orders you to present your hands and be cuffed.");

        amenu.addentry(ur_arrest, true, 'a', _("Allow yourself to be arrested."));
        amenu.addentry(ur_resist, true, 'r', _("Resist arrest!"));
        ///\EFFECT_INT >10 allows and increases chance whether you can feign death to avoid riot bot arrest
        if (foe->int_cur > 12 || (foe->int_cur > 10 && !one_in(foe->int_cur - 8))) {
            amenu.addentry(ur_trick, true, 't', _("Feign death."));
        }

        amenu.query();
        const int choice = amenu.ret;

        if (choice == ur_arrest) {
            z->anger = 0;

            item handcuffs("e_handcuffs", 0);
            handcuffs.charges = handcuffs.type->maximum_charges();
            handcuffs.active = true;
            handcuffs.set_var( "HANDCUFFS_X", foe->posx() );
            handcuffs.set_var( "HANDCUFFS_Y", foe->posy() );

            const bool is_uncanny = foe->has_active_bionic("bio_uncanny_dodge") && foe->power_level > 74 &&
                                    !one_in(3);
            ///\EFFECT_DEX >13 allows and increases chance to slip out of riot bot handcuffs
            const bool is_dex = foe->dex_cur > 13 && !one_in(foe->dex_cur - 11);

            if (is_uncanny || is_dex) {

                if (is_uncanny) {
                    foe->charge_power(-75);
                }

                add_msg(m_good,
                        _("You deftly slip out of the handcuffs just as the robot closes them.  The robot didn't seem to notice!"));
                foe->i_add(handcuffs);
            } else {
                handcuffs.item_tags.insert("NO_UNWIELD");
                foe->wield( foe->i_add( handcuffs ) );
                foe->moves -= 300;
                add_msg(_("The robot puts handcuffs on you."));
            }

            sounds::sound( z->pos(), 5,
                     _("You are under arrest, citizen.  You have the right to remain silent.  If you do not remain silent, anything you say may be used against you in a court of law."));
            sounds::sound( z->pos(), 5,
                     _("You have the right to an attorney.  If you cannot afford an attorney, one will be provided at no cost to you.  You may have your attorney present during any questioning."));
            sounds::sound( z->pos(), 5,
                     _("If you do not understand these rights, an officer will explain them in greater detail when taking you into custody."));
            sounds::sound( z->pos(), 5,
                     _("Do not attempt to flee or to remove the handcuffs, citizen.  That can be dangerous to your health."));

            z->moves -= 300;

            return true;
        }

        bool bad_trick = false;

        if (choice == ur_trick) {

            ///\EFFECT_INT >10 allows and increases chance of successful feign death against riot bot
            if (!one_in(foe->int_cur - 10)) {

                add_msg(m_good,
                        _("You fall to the ground and feign a sudden convulsive attack.  Though you're obviously still alive, the riotbot cannot tell the difference between your 'attack' and a potentially fatal medical condition.  It backs off, signaling for medical help."));

                z->moves -= 300;
                z->anger = -rng(0, 50);
                return true;
            } else {
                add_msg(m_bad, _("Your awkward movements do not fool the riotbot."));
                foe->moves -= 100;
                bad_trick = true;
            }
        }

        if ((choice == ur_resist) || bad_trick) {

            add_msg(m_bad, _("The robot sprays tear gas!"));
            z->moves -= 200;

            for( const tripoint &dest : g->m.points_in_radius( z->pos(), 2 ) ) {
                if( g->m.passable( dest ) &&
                    g->m.clear_path( z->pos(), dest, 3, 1, 100 ) ) {
                    g->m.add_field( dest, fd_tear_gas, rng(1, 3), 0 );
                }
            }

            return true;
        }

        return true;
    }

    if( calendar::once_every(5) ) {
        sounds::sound( z->pos(), 25, _("Empty your hands and hold your position, citizen!") );
    }

    if( dist > 5 && dist < 18 && one_in( 10 ) ) {

        z->moves -= 50;

        int delta = dist / 4 + 1;  //precautionary shot
        if( z->get_hp() < z->get_hp_max() ) {
            delta = 1;    //precision shot
        }

        tripoint dest( target->posx() + rng(0, delta) - rng(0, delta),
                       target->posy() + rng(0, delta) - rng(0, delta),
                       target->posz() );

        //~ Sound of a riotbot using its blinding flash
        sounds::sound( z->pos(), 3, _("fzzzzzt"));

        std::vector<tripoint> traj = line_to( z->pos(), dest, 0, 0 );
        for( auto &elem : traj ) {
            if( !g->m.trans( elem ) ) {
                break;
            }
            g->m.add_field( elem, fd_dazzling, 1, 0 );
        }
        return true;

    }

    return true;
}

bool mattack::bio_op_takedown(monster *z)
{
    if( !z->can_act() ) {
        return false;
    }

    Creature *target = z->attack_target();
    // TODO: Allow drop-takedown form above
    if( target == nullptr ||
        !is_adjacent( z, target, false ) ||
        !z->sees( *target ) ) {
        return false;
    }

    bool seen = g->u.sees( *z );
    player *foe = dynamic_cast< player* >( target );
    if( seen ) {
        add_msg(_("The %1$s mechanically grabs at %2$s!"), z->name().c_str(), target->disp_name().c_str() );
    }
    z->moves -= 100;

    if( target->uncanny_dodge() ) {
        return true;
    }

    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if( dodge_check(z, target) ) {
        target->add_msg_player_or_npc( _("You dodge it!"),
                                       _("<npcname> dodges it!") );
        target->on_dodge( z, z->type->melee_skill * 2 );
        return true;
    }
    int dam = rng(3, 9);
    if( foe == nullptr ) {
        // Handle mons earlier - less to check for
        dam = rng(6, 18); // Always aim for the torso
        target->deal_damage( z, bp_torso, damage_instance( DT_BASH, dam ) ); // Two hits - "leg" and torso
        target->deal_damage( z, bp_torso, damage_instance( DT_BASH, dam ) );
        target->add_effect( effect_downed, 3);
        if( seen ) {
            add_msg(_("%1$s slams %2$s to the ground!"), z->name().c_str(), target->disp_name().c_str() );
        }
        target->check_dead_state();
        return true;
    }
    // Yes, it has the CQC bionic.
    body_part hit = num_bp;
    if (one_in(2)) {
        hit = bp_leg_l;
    } else {
        hit = bp_leg_r;
    }
    // Weak kick to start with, knocks you off your footing

    // Literally "The zombie kicks" vvvvv |  FIXME FIX message or comment why Literally.
    //~ 1$s is bodypart name in accusative, 2$d is damage value.
    target->add_msg_if_player(m_bad, _("The zombie kicks your %1$s for %2$d damage..."),
                           body_part_name_accusative(hit).c_str(), dam);
    foe->deal_damage( z, hit, damage_instance( DT_BASH, dam ) );
    // At this point, Judo or Tentacle Bracing can make this much less painful
    if ( !foe->is_throw_immune()) {
        if( !target->is_immune_effect( effect_downed ) ) {
            if (one_in(4)) {
                hit = bp_head;
                dam = rng(9, 21); // 50% damage buff for the headshot.
                target->add_msg_if_player(m_bad, _("and slams you, face first, to the ground for %d damage!"), dam);
                foe->deal_damage( z, hit, damage_instance( DT_BASH, dam ) );
            } else {
                hit = bp_torso;
                dam = rng(6, 18);
                target->add_msg_if_player(m_bad, _("and slams you to the ground for %d damage!"), dam);
                foe->deal_damage( z, hit, damage_instance( DT_BASH, dam ) );
            }
            foe->add_effect( effect_downed, 3);
        }
    } else if( !thrown_by_judo(z) ) {
        // Saved by the tentacle-bracing! :)
        hit = bp_torso;
        dam = rng(3, 9);
        target->add_msg_if_player(m_bad, _("and slams you for %d damage!"), dam);
        foe->deal_damage( z, hit, damage_instance( DT_BASH, dam ) );
    }
    target->on_hit( z, hit,  z->type->melee_skill );
    foe->check_dead_state();

    return true;
}

bool mattack::suicide(monster *z)
{
    Creature *target = z->attack_target();
    if (!within_target_range(z, target, 2)) {
        return false;
    }
    z->die(z);

    return false;
}

bool mattack::kamikaze(monster *z)
{
    if (z->ammo.empty()) {
        // We somehow lost our ammo! Toggle this special off so we stop processing
        add_msg(m_debug, "Missing ammo in kamikaze special for %s.", z->name().c_str());
        z->disable_special("KAMIKAZE");
        return true;
    }

    // Get the bomb type and it's data
    const auto bomb_type = item::find_type( z->ammo.begin()->first );
    const itype* act_bomb_type;
    long charges;
    // Hardcoded data for charge variant items
    if (z->ammo.begin()->first == "mininuke") {
        act_bomb_type = item::find_type("mininuke_act");
        charges = 20;
    } else if (z->ammo.begin()->first == "c4") {
        act_bomb_type = item::find_type("c4armed");
        charges = 10;
    } else {
        auto usage = bomb_type->get_use( "transform" );
        if ( usage == nullptr ) {
            // Invalid item usage, Toggle this special off so we stop processing
            add_msg(m_debug, "Invalid bomb transform use in kamikaze special for %s.", z->name().c_str());
            z->disable_special("KAMIKAZE");
            return true;
        }
        const iuse_transform *actor = dynamic_cast<const iuse_transform *>( usage->get_actor_ptr() );
        if( actor == nullptr ) {
            // Invalid bomb item, Toggle this special off so we stop processing
            add_msg(m_debug, "Invalid bomb type in kamikaze special for %s.", z->name().c_str());
            z->disable_special("KAMIKAZE");
            return true;
        }
        act_bomb_type = item::find_type( actor->target );
        charges = actor->ammo_qty;
    }

    // HORRIBLE HACK ALERT! Remove the following code completely once we have working monster inventory processing
    if (z->has_effect( effect_countdown )) {
        if (z->get_effect( effect_countdown).get_duration() == 1) {
            z->die(nullptr);
            // Timer is out, detonate
            item i_explodes( act_bomb_type, calendar::turn, 0 );
            i_explodes.active = true;
            i_explodes.process(nullptr, z->pos(), false);
            return false;
        }
        return false;
    }
    // END HORRIBLE HACK

    auto use = act_bomb_type->get_use( "explosion" );
    if (use == nullptr ) {
        // Invalid active bomb item usage, Toggle this special off so we stop processing
        add_msg(m_debug, "Invalid active bomb explosion use in kamikaze special for %s.", z->name().c_str());
        z->disable_special("KAMIKAZE");
        return true;
    }
    const explosion_iuse *exp_actor = dynamic_cast<const explosion_iuse *>( use->get_actor_ptr() );
    if( exp_actor == nullptr ) {
        // Invalid active bomb item, Toggle this special off so we stop processing
        add_msg(m_debug, "Invalid active bomb type in kamikaze special for %s.", z->name().c_str());
        z->disable_special("KAMIKAZE");
        return true;
    }

    // Get our blast radius
    int radius = -1;
    if (exp_actor->fields_radius > radius) {
        radius = exp_actor->fields_radius;
    }
    if (exp_actor->emp_blast_radius > radius) {
        radius = exp_actor->emp_blast_radius;
    }
    // Extra check here to avoid sqrt if not needed
    if (exp_actor->explosion.power > -1) {
        int tmp = int(sqrt(double(exp_actor->explosion.power / 4)));
        if (tmp > radius) {
            radius = tmp;
        }
    }
    if( exp_actor->explosion.shrapnel.count > 0 ) {
        // Actual factor is 2 * radius, but figure most pieces of shrapnel will miss
        int tmp = int(sqrt(double(exp_actor->explosion.power / 4)));
        if (tmp > radius) {
            radius = tmp;
        }
    }
    // Flashbangs have a max range of 8
    if (exp_actor->do_flashbang && radius < 8) {
        radius = 8;
    }
    if (radius <= -1) {
        // Not a valid explosion size, toggle this special off to stop processing
        z->disable_special("KAMIKAZE");
        return true;
    }

    Creature *target = z->attack_target();
    if (target == nullptr) {
        return false;
    }
    // Range is (radius + distance they expect to gain on you during the countdown)
    // We double target speed because if the player is walking and then start to run their effective speed doubles
    // .65 factor was determined experimentally to be about the factor required for players to be able to *just barely*
    // outrun the explosion if they drop everything and run.
    float factor = float(z->get_speed()) / float(target->get_speed() * 2);
    int range = std::max(1, int(.65 * (radius + 1 + factor * charges)));

    // Check if we are in range to begin the countdown
    if (!within_target_range(z, target, range)) {
        return false;
    }

    // HORRIBLE HACK ALERT! Currently uses the amount of ammo as a pseduo-timer.
    // Once we have proper monster inventory item processing replace the following
    // line with the code below.
    z->add_effect( effect_countdown, charges + 1);
    /* Replacement code here for once we have working monster inventories

    item i_explodes(act_bomb_type->id, 0);
    i_explodes.charges = charges;
    z->add_item(i_explodes);
    z->disable_special("KAMIKAZE");
    */
    // END HORRIBLE HACK

    if (g->u.sees(z->pos())) {
        add_msg(m_bad, _("The %s lights up menacingly."), z->name().c_str() );
    }

    return true;
}

struct grenade_helper_struct
{
    std::string message = "";
    int chance = 1;
    float ammo_percentage = 1;
};

// Returns 0 if this should be retired, 1 if it was successful, and -1 if something went horribly wrong
int grenade_helper(monster *const z, Creature *const target, const int dist,
                     const int moves, std::map<std::string, grenade_helper_struct> data)
{
    // Can't do anything if we can't act
    if (!z->can_act()) {
        return 0;
    }
    // Too far or we can't target them
    if (!within_target_range(z, target, dist)) {
        return 0;
    }
    // We need an open space for these attacks
    auto const empty_neighbors = find_empty_neighbors(*z);
    size_t const empty_neighbor_count = empty_neighbors.second;
    if (!empty_neighbor_count) {
        return 0;
    }

    int total_ammo = 0;
    // Sum up the ammo entries to get a ratio.
    for( const auto &ammo_entry : z->type->starting_ammo ) {
        total_ammo += ammo_entry.second;
    }
    if( total_ammo == 0 ) {
        // Should never happen, but protect us from a div/0 if it does.
        return -1;
    }

    // Find how much ammo we currently have to get the total ratio
    int curr_ammo = 0;
    for (auto amm : z->ammo) {
        curr_ammo += amm.second;
    }
    float rat = curr_ammo / float(total_ammo);

    if (curr_ammo == 0) {
        // We've run out of ammo, get angry and toggle the special off.
        z->anger = 100;
        return -1;
    }

    // Hey look! another weighted list!
    // Grab all attacks that pass their chance check and we've spent enough ammo for
    weighted_float_list<std::string> possible_attacks;
    for (auto amm : z->ammo) {
        if (amm.second > 0 && data[amm.first].ammo_percentage >= rat) {
            possible_attacks.add(amm.first, 1.0 / data[amm.first].chance);
        }
    }
    std::string att = *possible_attacks.pick();

    z->moves -= moves;
    z->ammo[att]--;

    // if the player can see it
    if (g->u.sees(*z)) {
        if (data[att].message == "") {
            add_msg(m_debug, "Invalid ammo message in grenadier special.");
        } else {
            add_msg(m_bad, data[att].message.c_str(), z->name().c_str());
        }
    }

    // Get our monster type
    auto bomb_type = item::find_type(att);
    auto usage = bomb_type->get_use( "place_monster" );
    if (usage == nullptr ) {
        // Invalid bomb item usage, Toggle this special off so we stop processing
        add_msg(m_debug, "Invalid bomb item usage in grenadier special for %s.", z->name().c_str());
        return -1;
    }
    auto *actor = dynamic_cast<const place_monster_iuse *>( usage->get_actor_ptr() );
    if( actor == nullptr ) {
        // Invalid bomb item, Toggle this special off so we stop processing
        add_msg(m_debug, "Invalid bomb type in grenadier special for %s.", z->name().c_str());
        return -1;
    }

    const tripoint where = empty_neighbors.first[get_random_index(empty_neighbor_count)];

    if( g->summon_mon(actor->mtypeid, where ) ) {
        monster *hack = g->monster_at( where );
        hack->make_ally(z);
    }
    return 1;
}

bool mattack::grenadier(monster *const z)
{
    // Build our grenade map
    std::map<std::string, grenade_helper_struct> grenades;
    // Grenades
    grenades["bot_grenade_hack"].message = _("The %s fumbles open a pouch and a grenade hack flies out!");
    // Flashbangs
    grenades["bot_flashbang_hack"].message = _("The %s fumbles open a pouch and a flashbang hack flies out!");
    // Gasbombs
    grenades["bot_gasbomb_hack"].message = _("The %s fumbles open a pouch and a tear gas hack flies out!");
    // C-4
    grenades["bot_c4_hack"].message = _("The %s fumbles open a pouch and a C-4 hack flies out!");
    grenades["bot_c4_hack"].chance = 8;

    // Only can actively target the player right now. Once we have the ability to grab targets that we aren't
    // actively attacking change this to use that instead.
    Creature *const target = static_cast<Creature *>(&g->u);
    if (z->attitude_to( *target ) == Creature::A_FRIENDLY) {
        return false;
    }
    int ret = grenade_helper(z, target, 30, 60, grenades);
    if (ret == -1) {
        // Something broke badly, disable our special
        z->disable_special("GRENADIER");
    }
    return true;
}

bool mattack::grenadier_elite(monster *const z)
{
    // Build our grenade map
    std::map<std::string, grenade_helper_struct> grenades;
    // Grenades
    grenades["bot_grenade_hack"].message = _("The %s opens a pouch and a grenade hack flies out!");
    // Flashbangs
    grenades["bot_flashbang_hack"].message = _("The %s opens a pouch and a flashbang hack flies out!");
    // Gasbombs
    grenades["bot_gasbomb_hack"].message = _("The %s opens a pouch and a tear gas hack flies out!");
    // C-4
    grenades["bot_c4_hack"].message = _("The %s cackles and opens a pouch; a C-4 hack flies out!");
    grenades["bot_c4_hack"].chance = 8;
    grenades["bot_c4_hack"].ammo_percentage = .75;
    // Mininuke
    grenades["bot_mininuke_hack"].message = _("The %s opens its pack and spreads its hands, a mininuke hack floats out!!");
    grenades["bot_mininuke_hack"].chance = 50;
    grenades["bot_mininuke_hack"].ammo_percentage = .75;

    // Only can actively target the player right now. Once we have the ability to grab targets that we aren't
    // actively attacking change this to use that instead.
    Creature *const target = static_cast<Creature *>(&g->u);
    if (z->attitude_to( *target ) == Creature::A_FRIENDLY) {
        return false;
    }
    int ret = grenade_helper(z, target, 30, 60, grenades);
    if (ret == -1) {
        // Something broke badly, disable our special
        z->disable_special("GRENADIER_ELITE");
    }

    return true;
}

bool mattack::stretch_attack(monster *z)
{
    if( !z->can_act() ) {
        return false;
    }

    Creature *target = z->attack_target();
    if (target == nullptr ){
        return false;
    }

    int distance = rl_dist( z->pos(), target->pos() );
    if( distance < 2 || distance > 3 || !z->sees( *target ) ) {
        return false;
    }

    int dam = rng(5, 10);
    z->moves -= 100;
    for( auto &pnt : g->m.find_clear_path( z->pos(), target->pos() ) ) {
            if( g->m.impassable( pnt ) ) {
                add_msg( _( "The %1$s thrusts its arm at you but bounces off the %2$s" ), z->name().c_str(),
                         g->m.obstacle_name( pnt ).c_str() );
                return true;
            }
    }

    auto msg_type = target == &g->u ? m_warning : m_info;
    target->add_msg_player_or_npc(msg_type, _("The %s thrusts its arm at you, stretching to reach you from afar"),
                                         _("The %s thrusts its arm at <npcname>"),
                                        z->name().c_str());
    if (dodge_check(z, target) || g->u.uncanny_dodge()) {
        target->add_msg_player_or_npc(msg_type, _("You evade the stretched arm and it sails past you!"),
                                             _("<npcname> evades the stretched arm!"),
                                            z->name().c_str());
        target->on_dodge( z, z->type->melee_skill * 2 );
        //takes some time to retract the arm
        z->moves -= 150;
        return true;
    }

    body_part hit = target->get_random_body_part();
    dam = target->deal_damage( z, hit, damage_instance( DT_STAB, dam ) ).total_damage();

    if( dam > 0 ) {
        auto msg_type = target == &g->u ? m_bad : m_info;
        //~ 1$s is monster name, 2$s bodypart in accusative
        target->add_msg_player_or_npc( msg_type,
                                _("The %1$s's arm pierces your %2$s!"),
                                _("The %1$s arm pierces <npcname>'s %2$s!"),
                                z->name().c_str(),
                                body_part_name_accusative( hit ).c_str() );

        target->check_dead_state();
    } else {
        target->add_msg_player_or_npc( _("The %1$s arm hits your %2$s, but glances off your armor!"),
                                _("The %1$s hits <npcname>'s %2$s, but glances off armor!"),
                                z->name().c_str(),
                                body_part_name_accusative( hit ).c_str() );
    }

    target->on_hit( z, hit,  z->type->melee_skill );

    return true;
}

bool mattack::dodge_check( monster *z, Creature *target ) {
    ///\EFFECT_DODGE increases chance of dodging, vs their melee skill
    float dodge = std::max( target->get_dodge() - rng( 0, z->get_hit() ), 0.0f );
    if (rng(0, 10000) < 10000 / (1 + (99 * exp(-.6 * dodge)))) {
        return true;
    }
    return false;
}
