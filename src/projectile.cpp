#include "projectile.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "ammo_effect.h"
#include "calendar.h"
#include "character.h"
#include "condition.h"
#include "creature.h"
#include "creature_tracker.h"
#include "debug.h"
#include "dialogue.h"
#include "dialogue_helpers.h"
#include "effect_on_condition.h"
#include "enums.h"
#include "explosion.h"
#include "field.h"
#include "field_type.h"
#include "item.h"
#include "magic.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "messages.h"
#include "rng.h"
#include "string_formatter.h"
#include "talker.h"
#include "translations.h"
#include "type_id.h"

static const field_type_str_id field_fd_foamcrete( "fd_foamcrete" );

static const morale_type morale_pyromania_nofire( "morale_pyromania_nofire" );
static const morale_type morale_pyromania_startfire( "morale_pyromania_startfire" );

static const ter_str_id ter_t_foamcrete_floor( "t_foamcrete_floor" );
static const ter_str_id ter_t_foamcrete_wall( "t_foamcrete_wall" );

static const trait_id trait_PYROMANIA( "PYROMANIA" );

projectile::projectile() :
    critical_multiplier( 2.0 ), drop( nullptr ), custom_explosion( nullptr )
{ }

projectile::~projectile() = default;

projectile::projectile( projectile && ) noexcept( set_is_noexcept ) = default;

projectile::projectile( const projectile &other )
{
    *this = other;
}

projectile &projectile::operator=( const projectile &other )
{
    impact = other.impact;
    speed = other.speed;
    range = other.range;
    count = other.count;
    multishot = other.multishot;
    shot_spread = other.shot_spread;
    shot_impact = other.shot_impact;
    proj_effects = other.proj_effects;
    critical_multiplier = other.critical_multiplier;
    set_drop( other.get_drop() );
    set_custom_explosion( other.get_custom_explosion() );

    return *this;
}

const item &projectile::get_drop() const
{
    if( drop != nullptr ) {
        return *drop;
    }

    static const item null_drop;
    return null_drop;
}

void projectile::set_drop( const item &it )
{
    if( it.is_null() ) {
        unset_drop();
    } else {
        drop = std::make_unique<item>( it );
    }
}

void projectile::set_drop( item &&it )
{
    if( it.is_null() ) {
        unset_drop();
    } else {
        drop = std::make_unique<item>( std::move( it ) );
    }
}

void projectile::unset_drop()
{
    drop.reset( nullptr );
}

const explosion_data &projectile::get_custom_explosion() const
{
    if( custom_explosion != nullptr ) {
        return *custom_explosion;
    }

    static const explosion_data null_explosion{};
    return null_explosion;
}

void projectile::set_custom_explosion( const explosion_data &ex )
{
    custom_explosion = std::make_unique<explosion_data>( ex );
}

void projectile::unset_custom_explosion()
{
    custom_explosion.reset();
}

static void foamcrete_build( const tripoint_bub_ms &p )
{
    map &here = get_map();

    if( !( ter_t_foamcrete_wall.is_valid() && ter_t_foamcrete_floor.is_valid() &&
           field_fd_foamcrete.is_valid() ) ) {
        debugmsg( "Foamcrete terrains or fields are missing" );
        return;
    }

    if( here.has_flag_ter( ter_furn_flag::TFLAG_NO_FLOOR, p ) ) {
        for( const tripoint_bub_ms &ep : here.points_in_radius( p, 1 ) ) {
            if( here.has_flag_ter( ter_furn_flag::TFLAG_SUPPORTS_ROOF, ep ) ) {
                here.ter_set( p, ter_t_foamcrete_floor );
                here.add_field( p, field_fd_foamcrete, 1 );
                return;
            }
        }
        add_msg( m_bad, _( "The foamcrete falls without a wall to anchor against." ) );
    } else if( here.get_field_intensity( p, field_fd_foamcrete ) >= 2 ) {
        here.bash( p, 9001, false, true, false );
        here.ter_set( p, ter_t_foamcrete_wall );
    } else {
        here.add_field( p, field_fd_foamcrete, 1 );
    }
}

void apply_ammo_effects( Creature *source, const tripoint_bub_ms &p,
                         const std::set<ammo_effect_str_id> &effects, const int dealt_damage )
{
    map &here = get_map();
    Character &player_character = get_player_character();

    for( const ammo_effect &ae : ammo_effects::get_all() ) {
        if( !one_in( ae.trigger_chance ) ) {
            continue;
        }
        if( effects.count( ae.id ) > 0 ) {
            for( const tripoint_bub_ms &pt : here.points_in_radius( p, ae.aoe_radius,
                    ae.aoe_radius_z ) ) {
                if( x_in_y( ae.aoe_chance, 100 ) ) {
                    const bool check_sees = !ae.aoe_check_sees || here.sees( p, pt, ae.aoe_check_sees_radius );
                    const bool check_passable = !ae.aoe_check_passable || here.passable( pt );
                    if( check_sees && check_passable ) {
                        here.add_field( pt, ae.aoe_field_type, rng( ae.aoe_intensity_min, ae.aoe_intensity_max ) );

                        if( player_character.has_trait( trait_PYROMANIA ) &&
                            !player_character.has_morale( morale_pyromania_startfire ) ) {
                            for( const auto &fd : here.field_at( pt ) ) {
                                if( fd.first->has_fire ) {
                                    player_character.add_msg_if_player( m_good,
                                                                        _( "You feel a surge of euphoria as flames burst out!" ) );
                                    player_character.add_morale( morale_pyromania_startfire, 15, 15, 8_hours, 6_hours );
                                    player_character.rem_morale( morale_pyromania_nofire );
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            if( ae.aoe_explosion_data.power > 0 ) {
                explosion_handler::explosion( source, p, ae.aoe_explosion_data );
            }
            if( ae.do_flashbang ) {
                explosion_handler::flashbang( p );
            }
            if( ae.do_emp_blast ) {
                explosion_handler::emp_blast( p );
            }
            if( ae.foamcrete_build ) {
                foamcrete_build( p );
            }

            //run EoCs
            for( const effect_on_condition_id &eoc : ae.eoc ) {
                Creature *critter = get_creature_tracker().creature_at( p );
                dialogue d( get_talker_for( *source ), critter == nullptr ? nullptr : get_talker_for( critter ) );
                // `p` is tripoint relative to the upper left corner of currently loaded overmap
                // not very useful for player's purposes methinks, but much appreciated
                // write_var_value( var_type::context, "proj_target_tripoint", &d, p.abs().to_string());
                write_var_value( var_type::context, "proj_damage", &d, dealt_damage );
                eoc->activate( d );
            }
            //cast ammo effect spells
            const spell ammo_spell = ae.spell_data.get_spell();
            if( ammo_spell.is_valid() ) {
                if( ae.always_cast_spell || dealt_damage > 0 ) {
                    ammo_spell.cast_all_effects( *const_cast<Creature *>( source ), p );
                    ammo_spell.make_sound( p, *const_cast<Creature *>( source ) );
                }
            }
        }
    }
}

int max_aoe_size( const std::set<ammo_effect_str_id> &tags )
{
    int aoe_size = 0;
    for( const ammo_effect &aed : ammo_effects::get_all() ) {
        if( tags.count( aed.id ) > 0 ) {
            aoe_size = std::max( aoe_size,  aed.aoe_size ) ;
        }
    }
    return aoe_size;
}

void multi_projectile_hit_message( Creature *critter, int hit_count, int damage_taken,
                                   const std::string &projectile_name )
{
    const map &here = get_map();

    if( hit_count > 0 && get_player_character().sees( here, *critter ) ) {
        // Building a phrase to summarize the fragment effects.
        // Target, Number of impacts, total amount of damage, proportion of deflected fragments.
        std::map<int, std::string> impact_count_descriptions = {
            { 1, _( "a" ) }, { 2, _( "several" ) }, { 5, _( "many" ) },
            { 20, _( "a large number of" ) }, { 100, _( "a huge number of" ) },
            { std::numeric_limits<int>::max(), _( "an immense number of" ) }
        };
        std::string impact_count = std::find_if(
                                       impact_count_descriptions.begin(), impact_count_descriptions.end(),
        [hit_count]( const std::pair<int, std::string> &desc ) {
            return desc.first >= hit_count;
        } )->second;
        std::string damage_description = ( damage_taken > 0 ) ?
                                         string_format( _( "dealing %d damage" ), damage_taken ) :
                                         _( "but they deal no damage" );
        if( critter->is_avatar() ) {
            //~ Phrase describing getting hit by multiple projectiles, i.e. "You are hit by many bomb fragments, dealing 50 damage."
            add_msg( _( "You are hit by %s %s, %s." ), impact_count, projectile_name, damage_description );
        } else if( critter->is_npc() ) {
            //~ Phrase describing getting hit by multiple projectiles, i.e. "Fred Johnson is hit by many bomb fragments, dealing 50 damage."
            critter->add_msg_if_npc( _( "<npcname> is hit by %s %s, %s." ), impact_count, projectile_name,
                                     damage_description );
        } else {
            //~ Phrase describing getting hit by multiple projectiles, i.e. "The zombie is hit by many bomb fragments, dealing 50 damage."
            add_msg( _( "%s is hit by %s %s, %s." ), critter->disp_name( false, true ), impact_count,
                     projectile_name, damage_description );
        }
    }
}
