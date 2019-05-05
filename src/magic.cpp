#include "magic.h"

#include "calendar.h"
#include "color.h"
#include "damage.h"
#include "game.h"
#include "map.h"
#include "mapdata.h"
#include "messages.h"
#include "monster.h"
#include "mutation.h"
#include "npc.h"
#include "options.h"
#include "player.h"
#include "projectile.h"
#include "rng.h"
#include "translations.h"

// LOADING
// spell_type

namespace
{
generic_factory<spell_type> spell_factory( "spell" );
}

template<>
const spell_type &string_id<spell_type>::obj() const
{
    return spell_factory.obj( *this );
}

template<>
bool string_id<spell_type>::is_valid() const
{
    return spell_factory.is_valid( *this );
}

void spell_type::load_spell( JsonObject &jo, const std::string &src )
{
    spell_factory.load( jo, src );
}

energy_type energy_source_from_string( std::string str )
{
    if( str == "MANA" ) {
        return mana_energy;
    } else if( str == "HP" ) {
        return hp_energy;
    } else if( str == "BIONIC" ) {
        return bionic_energy;
    } else if( str == "STAMINA" ) {
        return stamina_energy;
    } else {
        return none_energy;
    }
}

damage_type damage_type_from_string( std::string str )
{
    if( str == "fire" ) {
        return DT_HEAT;
    } else if( "acid" ) {
        return DT_ACID;
    } else if( "bash" ) {
        return DT_BASH;
    } else if( "bio" ) {
        return DT_BIOLOGICAL;
    } else if( "cold" ) {
        return DT_COLD;
    } else if( "cut" ) {
        return DT_CUT;
    } else if( "electric" ) {
        return DT_ELECTRIC;
    } else if( "stab" ) {
        return DT_STAB;
    } else {
        return DT_TRUE;
    }
}

std::vector<valid_target> strings_to_targets( std::vector<std::string> targets_strings )
{
    std::vector<valid_target> targets;
    for( std::string str : targets_strings ) {
        if( str == "ally" ) {
            targets.emplace_back( target_ally );
        } else if( str == "ground" ) {
            targets.emplace_back( target_ground );
        } else if( str == "hostile" ) {
            targets.emplace_back( target_hostile );
        } else if( str == "self" ) {
            targets.emplace_back( target_self );
        } else if( str == "none" ){
            targets.emplace_back( target_none );
        } else {
            debugmsg( "Error loading spell. %s is not an available valid_target", str );
        }
    }
    return targets;
}

void spell_type::load( JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "id", id );
    mandatory( jo, was_loaded, "name", name, translated_string_reader );
    mandatory( jo, was_loaded, "description", description, translated_string_reader );
    mandatory( jo, was_loaded, "effect", effect );
    std::vector<std::string> valid_targets;
    mandatory( jo, was_loaded, "valid_targets", valid_targets );
    this->valid_targets = strings_to_targets( valid_targets );

    optional( jo, was_loaded, "effect_str", effect_str, "" );

    optional( jo, was_loaded, "min_damage", min_damage, 0 );
    optional( jo, was_loaded, "damage_increment", damage_increment, 0.0f );
    optional( jo, was_loaded, "max_damage", max_damage, 0 );

    optional( jo, was_loaded, "min_range", min_range, 0 );
    optional( jo, was_loaded, "range_increment", range_increment, 0.0f );
    optional( jo, was_loaded, "max_range", max_range, 0 );

    optional( jo, was_loaded, "min_aoe", min_aoe, 0 );
    optional( jo, was_loaded, "aoe_increment", aoe_increment, 0.0f );
    optional( jo, was_loaded, "max_aoe", max_aoe, 0 );

    optional( jo, was_loaded, "min_dot", min_dot, 0 );
    optional( jo, was_loaded, "dot_increment", dot_increment, 0.0f );
    optional( jo, was_loaded, "max_dot", max_dot, 0 );

    optional( jo, was_loaded, "min_duration", min_duration, 0 );
    optional( jo, was_loaded, "duration_increment", duration_increment, 0.0f );
    optional( jo, was_loaded, "max_duration", max_duration, 0 );

    optional( jo, was_loaded, "min_pierce", min_pierce, 0 );
    optional( jo, was_loaded, "pierce_increment", pierce_increment, 0.0f );
    optional( jo, was_loaded, "max_pierce", max_pierce, 0 );

    optional( jo, was_loaded, "base_energy_cost", base_energy_cost, 0 );
    std::string temp_string;
    optional( jo, was_loaded, "spell_class", temp_string, "NONE" );
    spell_class = trait_id( temp_string );
    optional( jo, was_loaded, "energy_source", temp_string, "NONE" );
    energy_source = energy_source_from_string( temp_string );
    optional( jo, was_loaded, "damage_type", temp_string, "NONE" );
    dmg_type = damage_type_from_string( temp_string );
    optional( jo, was_loaded, "difficulty", difficulty, 0 );
    optional( jo, was_loaded, "max_level", max_level, 0 );
    optional( jo, was_loaded, "base_casting_time", base_casting_time, 0 );

}

void spell_type::check_consistency()
{
    for( const spell_type &sp_t : get_all() ) {
        if( sp_t.min_aoe > sp_t.max_aoe ) {
            debugmsg( string_format( "ERROR: %s has higher min_aoe than max_aoe", sp_t.id.c_str() ) );
        }
        if( abs( sp_t.min_damage ) > abs( sp_t.max_damage ) ) {
            debugmsg( string_format( "ERROR: %s has higher min_damage than max_damage", sp_t.id.c_str() ) );
        }
        if( sp_t.min_range > sp_t.max_range ) {
            debugmsg( string_format( "ERROR: %s has higher min_range than max_range", sp_t.id.c_str() ) );
        }
        if( sp_t.min_dot > sp_t.max_dot ) {
            debugmsg( string_format( "ERROR: %s has higher min_dot than max_dot", sp_t.id.c_str() ) );
        }
        if( sp_t.min_duration > sp_t.max_duration ) {
            debugmsg( string_format( "ERROR: %s has higher min_dot_time than max_dot_time", sp_t.id.c_str() ) );
        }
        if( sp_t.min_pierce > sp_t.max_pierce ) {
            debugmsg( string_format( "ERROR: %s has higher min_pierce than max_pierce", sp_t.id.c_str() ) );
        }
    }
}

const std::vector<spell_type> &spell_type::get_all()
{
    return spell_factory.get_all();
}

void spell_type::reset_all()
{
    spell_factory.reset();
}

bool spell_type::is_valid() const
{
    return spell_factory.is_valid( this->id );
}

// spell

spell::spell( const spell_type *sp, player *p )
{
    type = sp;
    owner = p;
}

spell::spell( spell_id sp, player *p, int xp )
{
    type = &sp.obj();
    owner = p;
    experience = xp;
}

spell_id spell::id() const
{
    return type->id;
}

int spell::damage() const
{
    if( type->min_damage >= 0 ) {
        return std::min( static_cast<int>( type->min_damage + round( get_level() *
                                           type->damage_increment ) ), type->max_damage );
    } else { // if it's negative, min and max work differently
        return std::max( static_cast<int>( type->min_damage + round( get_level() *
                                           type->damage_increment ) ), type->max_damage );
    }
}

int spell::aoe() const
{
    return std::min( static_cast<int>( type->min_aoe + round( get_level() * type->damage_increment ) ),
                     type->max_aoe );
}

int spell::range() const
{
    return std::min( static_cast<int>( type->min_range + round( get_level() * type->range_increment ) ),
                     type->max_range );
}

int spell::duration() const
{
    return std::min( static_cast<int>( type->min_duration + round( get_level() *
                                       type->duration_increment ) ),
                     type->max_duration );
}

time_duration spell::duration_turns() const
{
    return 1_turns * duration() / 100;
}

void spell::gain_level()
{
    gain_exp( exp_to_next_level() );
}
/*
void spell::lose_level()
{
    if( level > 0 ) {
        level--;
    }
}
*/
bool spell::is_max_level() const
{
    return get_level() >= type->max_level;
}

bool spell::can_learn() const
{
    if( type->spell_class == trait_id( "NONE" ) ) {
        return true;
    }
    return owner->has_trait( type->spell_class );
}

int spell::energy_cost() const
{
    // todo: formula to vary energy cost
    return type->base_energy_cost;
}

bool spell::can_cast() const
{
    if( !owner->knows_spell( type->id ) ) {
        // how in the world can this happen?
        debugmsg( "ERROR: owner of spell does not know spell" );
        return false;
    }
    switch( type->energy_source ) {
        case mana_energy:
            return owner->available_mana() >= energy_cost();
        case stamina_energy:
            return owner->stamina >= energy_cost();
        case hp_energy: {
            for( int i = 0; i < num_hp_parts; i++ ) {
                if( energy_cost() < owner->hp_cur[i] ) {
                    return true;
                }
            }
        }
        case bionic_energy:
            return owner->power_level >= energy_cost();
        case none_energy:
        default:
            return true;
    }
}

int spell::modded_int() const
{
    return owner->int_cur;
}

int spell::spellcraft_level() const
{
    return owner->get_skill_level( skill_id( "spellcraft" ) );
}

int spell::get_difficulty() const
{
    return type->difficulty;
}

int spell::casting_time() const
{
    // todo: formula for casting time
    return type->base_casting_time;
}

std::string spell::name() const
{
    return _( type->name );
}

float spell::spell_fail() const
{
    // formula is based on the following:
    // exponential curve
    // effective skill of 0 or less is 100% failure
    // effective skill of 8 (8 int, 0 spellcraft, 0 spell level, spell difficulty 0) is ~50% failure
    // effective skill of 30 is 0% failure
    const float effective_skill = 2 * ( get_level() - get_difficulty() ) + modded_int() +
                                  spellcraft_level();
    // add an if statement in here because sufficiently large numbers will definitely overflow because of exponents
    if( effective_skill > 30.0f ) {
        return 0.0f;
    } else if( effective_skill < 0.0f ) {
        return 1.0f;
    }
    const float fail_chance = pow( ( effective_skill - 30.0f ) / 30.0f, 2 );
    return clamp( fail_chance, 0.0f, 1.0f );
}

std::string spell::colorized_fail_percent() const
{
    const float fail_fl = spell_fail() * 100.0f;
    std::string fail_str;
    fail_fl == 100.0f ? fail_str = _( "Difficult!" ) : fail_str = string_format( "%.1f %s %s", fail_fl,
                                   "%",  _( "Failure Chance" ) );
    nc_color color;
    if( fail_fl > 90.0f ) {
        color = c_magenta;
    } else if( fail_fl > 75.0f ) {
        color = c_red;
    } else if( fail_fl > 60.0f ) {
        color = c_light_red;
    } else if( fail_fl > 35.0f ) {
        color = c_yellow;
    } else if( fail_fl > 15.0f ) {
        color = c_green;
    } else {
        color = c_light_green;
    }
    return colorize( fail_str, color );
}

int spell::xp() const
{
    return experience;
}

void spell::gain_exp( int nxp )
{
    experience += nxp;
}

std::string spell::energy_string() const
{
    switch( type->energy_source ) {
        case hp_energy:
            return _( "health" );
        case mana_energy:
            return _( "mana" );
        case stamina_energy:
            return _( "stamina" );
        case bionic_energy:
            return _( "bionic power" );
        default:
            return "";
    }
}

std::string spell::energy_cost_string( const player &p ) const
{
    if( energy_source() == none_energy ) {
        return _( "none" );
    }
    if( energy_source() == bionic_energy || energy_source() == mana_energy ) {
        return colorize( to_string( energy_cost() ), c_light_blue );
    }
    if( energy_source() == hp_energy ) {
        auto pair = get_hp_bar( energy_cost(), p.get_hp_max() / num_hp_parts );
        return colorize( pair.first, pair.second );
    }
    if( energy_source() == stamina_energy ) {
        auto pair = get_hp_bar( energy_cost(), p.get_stamina_max() );
        return colorize( pair.first, pair.second );
    }
    return "error: energy_type";
}

std::string spell::energy_cur_string( const player &p ) const
{
    if( energy_source() == none_energy ) {
        return _( "infinite" );
    }
    if( energy_source() == bionic_energy ) {
        return colorize( to_string( p.power_level ), c_light_blue );
    }
    if( energy_source() == mana_energy ) {
        return colorize( to_string( p.available_mana() ), c_light_blue );
    }
    if( energy_source() == stamina_energy ) {
        auto pair = get_hp_bar( p.stamina, p.get_stamina_max() );
        return colorize( pair.first, pair.second );
    }
    if( energy_source() == hp_energy ) {
        return "";
    }
    return "error: energy_type";
}

bool spell::is_valid() const
{
    if( type == nullptr ) {
        return false;
    }
    return type->is_valid();
}

std::string spell::effect() const
{
    return type->effect;
}

energy_type spell::energy_source() const
{
    return type->energy_source;
}

bool spell::is_valid_target( valid_target t ) const
{
    for( valid_target tar : type->valid_targets ) {
        if( tar == t ) {
            return true;
        }
    }
    return false;
}

bool spell::is_valid_target( const tripoint &p ) const
{
    bool valid = false;
    if( Creature *const cr = g->critter_at<Creature>( p ) ) {
        Creature::Attitude cr_att = cr->attitude_to( g->u );
        valid = valid || ( cr_att != Creature::A_FRIENDLY && is_valid_target( target_hostile ) ) ||
                ( cr_att == Creature::A_FRIENDLY && is_valid_target( target_ally ) );
    }
    if( p == g->u.pos() ) {
        valid = valid || is_valid_target( target_self );
    }
    return valid || is_valid_target( target_ground );
}

std::string spell::description() const
{
    return _( type->description );
}

nc_color spell::damage_type_color() const
{
    return c_red;
}

// constants defined below are just for the formula to be used,
// in order for the inverse formula to be equivalent
static const float a = 6200.0;
static const float b = 0.146661;
static const float c = -62.5;

int spell::get_level() const
{
    // you aren't at the next level unless you have the requisite xp, so floor
    return std::max( static_cast<int>( floor( log( experience + a ) / b + c ) ), 0 );
}

// helper function to calculate xp needed to be at a certain level
// pulled out as a helper function to make it easier to either be used in the future
// or easier to tweak the formula
int exp_for_level( int level )
{
    // level 0 never needs xp
    if( level == 0 ) {
        return 0;
    }
    return round( exp( ( level - c ) * b ) ) - a;
}

int spell::exp_to_next_level() const
{
    return exp_for_level( get_level() + 1 ) - xp();
}

std::string spell::exp_progress() const
{
    const int level = get_level();
    const int this_level_xp = exp_for_level( level );
    const int next_level_xp = exp_for_level( level + 1 );
    const int denominator = next_level_xp - this_level_xp;
    const float progress = static_cast<float>( xp() - this_level_xp ) / std::max( 1.0f,
                           static_cast<float>( denominator ) );
    return string_format( "%i%c", clamp( static_cast<int>( round( progress * 100 ) ), 0, 99 ), '%' );
}

float spell::exp_modifier() const
{
    const float int_modifier = ( owner->get_int() - 8.0f ) / 8.0f;
    const float difficulty_modifier = get_difficulty() / 20.0f;
    const float spellcraft_modifier = owner->get_skill_level( skill_id( "SPELLCRAFT" ) ) / 10.0f;

    return int_modifier + difficulty_modifier + spellcraft_modifier + 1.0f;
}

int spell::casting_exp() const
{
    // the amount of xp you would get with no modifiers
    const int base_casting_xp = 75;

    return round( owner->adjust_for_focus( base_casting_xp * exp_modifier() ) );
}

damage_type spell::dmg_type() const
{
    return type->dmg_type;
}

damage_instance spell::get_damage_instance() const
{
    damage_instance dmg;
    dmg.add_damage( dmg_type(), damage() );
    return dmg;
}

dealt_damage_instance spell::get_dealt_damage_instance() const
{
    dealt_damage_instance dmg;
    dmg.set_damage( dmg_type(), damage() );
    add_msg( "%i", damage() );
    return dmg;
}

std::string spell::effect_data() const
{
    return type->effect_str;
}

int spell::heal( const tripoint &target ) const
{
    monster *const mon = g->critter_at<monster>( target );
    if( mon ) {
        return mon->heal( -damage() );
    }
    player *const p = g->critter_at<player>( target );
    if( p ) {
        p->healall( -damage() );
        return -damage();
    }
    return -1;
}

// player

bool player::knows_spell( const std::string &sp ) const
{
    return knows_spell( spell_id( sp ) );
}

bool player::knows_spell( spell_id sp ) const
{
    return spellbook.count( sp ) == 1;
}

void player::learn_spell( const std::string &sp, bool force )
{
    learn_spell( spell_id( sp ), force );
}

void player::learn_spell( spell_id sp, bool force )
{
    learn_spell( &sp.obj(), force );
}

void player::learn_spell( const spell_type *sp, bool force )
{
    if( !sp->is_valid() ) {
        debugmsg( "Tried to learn invalid spell" );
        return;
    }
    spell new_spell( sp, this );
    if( !new_spell.is_valid() ) {
        debugmsg( "Tried to learn invalid spell" );
        return;
    }
    const bool no_spell_class = sp->spell_class == trait_id( "NONE" );
    bool has_trait_cat = false;
    // NONE isn't really a trait, but a placeholder for magic spells.
    // as such, we don't really want it in json, so we need to check if
    // that's the one we're dealing with first.
    if( !no_spell_class ) {
        has_trait_cat = has_opposite_trait( sp->spell_class );
    }
    if( force || new_spell.can_learn() || no_spell_class || !has_trait_cat ) {
        if( !force && !no_spell_class && !has_trait_cat ) {
            if( query_yn(
                    _( "Learning this spell will make you a %s and lock you out of other unique spells.\nContinue?" ),
                    sp->spell_class.obj().name() ) ) {
                set_mutation( sp->spell_class );
                add_msg_if_player( sp->spell_class.obj().desc() );
            } else {
                return;
            }
        }
        spellbook.emplace( sp->id, new_spell );
        add_msg_if_player( m_good, _( "You learned %s!" ), _( sp->name ) );
    }
}

void player::forget_spell( const std::string &sp )
{
    forget_spell( spell_id( sp ) );
}

void player::forget_spell( spell_id sp )
{
    if( !knows_spell( sp ) ) {
        debugmsg( "Can't forget a spell you don't know!" );
        return;
    }
    spellbook.erase( sp );
}

bool player::can_learn_spell( spell_id sp ) const
{
    const spell_type sp_t = sp.obj();
    if( sp_t.spell_class == trait_id( "NONE" ) ) {
        return true;
    }
    return !has_opposite_trait( sp_t.spell_class );
}

spell &player::get_spell( spell_id sp )
{
    if( !knows_spell( sp ) ) {
        debugmsg( "ERROR: Tried to get unknown spell" );
    }
    spell &temp_spell = spellbook[ sp ];
    return temp_spell;
}

int player::available_mana() const
{
    return mana;
}

void player::set_mana( int new_mana )
{
    mana = new_mana;
}

void player::mod_mana( int add_mana )
{
    set_mana( clamp( mana + add_mana, ( int )0, max_mana() ) );
}

int player::max_mana() const
{
    const float int_bonus = ( ( 0.2f + get_int() * 0.1f ) - 1.0f ) * mana_base;
    return mana_base + int_bonus;
}

void player::update_mana( float turns )
{
    // mana should replenish in 8 hours.
    const float full_replenish = to_turns<float>( 8_hours );
    const float ratio = turns / full_replenish;
    mod_mana( floor( ratio * max_mana() ) );
}

std::vector<spell_id> player::spells() const
{
    std::vector<spell_id> spell_ids;
    for( auto pair : spellbook ) {
        spell_ids.emplace_back( pair.first );
    }
    return spell_ids;
}

// does the player have enough energy (of the type of the spell) to cast the spell?
bool player::has_enough_energy( spell sp ) const
{
    int cost = sp.energy_cost();
    switch( sp.energy_source() ) {
        case mana_energy:
            return available_mana() >= cost;
        case bionic_energy:
            return power_level >= cost;
        case stamina_energy:
            return stamina >= cost;
        case hp_energy:
            for( int i = 0; i < num_hp_parts; i++ ) {
                if( hp_cur[i] > cost ) {
                    return true;
                }
            }
            return false;
        case none_energy:
            return true;
        default:
            return false;
    }
}

int player::time_to_learn_spell( const std::string &str ) const
{
    return time_to_learn_spell( spell_id( str ) );
}

int player::time_to_learn_spell( spell_id sp ) const
{
    assert( !knows_spell( sp ) );
    const int THIRTY_MINUTES = 30000;
    return THIRTY_MINUTES * ( sp.obj().difficulty * ( 1.0 + ( get_int() - 8.0 ) / 8.0 ) +
                              ( get_skill_level( skill_id( "SPELLCRAFT" ) ) / 10.0 ) );
}

// spell_effect

namespace spell_effect
{

static tripoint random_point( int min_distance, int max_distance, const tripoint &player_pos )
{
    const int angle = rng( 0, 360 );
    const int dist = rng( min_distance, max_distance );
    const int x = round( dist * cos( angle ) );
    const int y = round( dist * sin( angle ) );
    return tripoint( x+player_pos.x, y+player_pos.y, player_pos.z );
}

void teleport( int min_distance, int max_distance )
{
    if( min_distance > max_distance || min_distance < 0 || max_distance < 0 ) {
        debugmsg( "ERROR: Teleport argument(s) invalid" );
        return;
    }
    const tripoint player_pos = g->u.pos();
    tripoint target;
    // limit the loop just in case it's impossble to find a valid point in the range
    int tries = 0;
    do {
        target = random_point( min_distance, max_distance, player_pos );
        tries++;
    } while( g->m.impassable( target ) && tries < 20 );
    if( tries == 20 ) {
        add_msg( m_bad, _( "Unable to vind a valid target for teleport." ) );
        return;
    }
    g->place_player( target );
}

void pain_split()
{
    player &p = g->u;
    add_msg( m_info, "Your injuries even out." );
    int num_limbs = 0; // number of limbs effected (broken don't count)
    int total_hp = 0; // total hp among limbs

    for( const int &part : p.hp_cur ) {
        if( part != 0 ) {
            num_limbs++;
            total_hp += part;
        }
    }
    for( int &part : p.hp_cur ) {
        const int hp_each = total_hp / num_limbs;
        if( part != 0 ) {
            part = hp_each;
        }
    }
}

void shallow_pit( const tripoint &target )
{
    g->m.ter_set( target, t_pit_shallow );
    add_msg( m_info, _( "The earth moves out of the way for you" ) );
}

static bool in_spell_aoe( const tripoint &target, const tripoint &epicenter, const int &radius, const bool ignore_walls )
{
    return true;
}

// spells do not reduce in damage the further away from the epicenter the targets are
// rather they do their full damage in the entire area of effect
static std::vector<tripoint> spell_effect_area( spell &sp, const tripoint &target, bool ignore_walls = false )
{
    std::vector<tripoint> targets = { target }; // initialize with epicenter
    if( sp.aoe() <= 1 ) {
        return targets;
    }

    const int aoe_radius = sp.aoe();
    for( int x = target.x - aoe_radius; x < target.x + aoe_radius; x++ ) {
        for( int y = target.y - aoe_radius; y < target.y + aoe_radius; y++ ) {
            for( int z = target.z - aoe_radius; y < target.z + aoe_radius; z++ ) {
                const tripoint potential_target( x, y, z );
                if( in_spell_aoe( potential_target, target, aoe_radius, ignore_walls ) ) {
                    targets.emplace_back( potential_target );
                }
            }
        }
    }

    // Draw the explosion
    std::map<tripoint, nc_color> explosion_colors;
    for( auto &pt : targets ) {
        if( g->m.impassable( pt ) ) {
            continue;
        }
        explosion_colors[pt] = sp.damage_type_color();
    }

    g->draw_custom_explosion( g->u.pos(), explosion_colors );
    return targets;
}

void target_attack( spell &sp, const tripoint &epicenter )
{
    for( const tripoint target : spell_effect_area( sp, epicenter, true ) )
    {
        Creature *const cr = g->critter_at<Creature>( target );
        if( !cr ) {
            continue;
        }

        projectile bolt;
        bolt.impact = sp.get_damage_instance();
        bolt.proj_effects.emplace( "magic" );

        dealt_projectile_attack atk;
        atk.end_point = target;
        atk.hit_critter = cr;
        atk.proj = bolt;
        if( sp.damage() > 0 ) {
            cr->deal_projectile_attack( &g->u, atk, true );
        } else {
            sp.heal( target );
            add_msg( m_good, _( "%s wounds are closing up!" ), cr->disp_name( true ) );
        }
        if( !sp.effect_data().empty() ) {
            const int dur_moves = sp.duration();
            const time_duration dur_td = 1_turns * dur_moves / 100;
            cr->add_effect( efftype_id( sp.effect_data() ), dur_td, bp_torso );
        }
    }
}

void spawn_ethereal_item( spell &sp )
{
    item granted( sp.effect_data(), calendar::turn );
    if( !granted.is_comestible() ) {
        granted.set_rot( -sp.duration_turns() );
        granted.set_flag( "ETHEREAL_ITEM" );
    }
    if( granted.count_by_charges() && sp.damage() > 0 ) {
        granted.charges = sp.damage();
    }
    if( g->u.can_wear( granted ).success() ) {
        granted.set_flag( "FIT" );
        g->u.wear_item( granted, false );
    } else {
        g->u.weapon = granted;
    }
    if( !granted.count_by_charges() ) {
        for( int i = 1; i < sp.damage(); i++ ) {
            g->u.i_add( granted );
        }
    }
}

}
