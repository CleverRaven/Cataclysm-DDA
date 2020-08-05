#include "magic.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <set>
#include <tuple>
#include <utility>

#include "calendar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character.h"
#include "color.h"
#include "compatibility.h"
#include "creature.h"
#include "cursesdef.h"
#include "damage.h"
#include "debug.h"
#include "enum_conversions.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "field.h"
#include "game.h"
#include "generic_factory.h"
#include "input.h"
#include "inventory.h"
#include "item.h"
#include "json.h"
#include "line.h"
#include "magic_enchantment.h"
#include "map.h"
#include "messages.h"
#include "mongroup.h"
#include "monster.h"
#include "mtype.h"
#include "mutation.h"
#include "output.h"
#include "character.h"
#include "pldata.h"
#include "point.h"
#include "rng.h"
#include "sounds.h"
#include "string_formatter.h"
#include "string_id.h"
#include "translations.h"
#include "ui.h"
#include "units.h"

static const trait_id trait_NONE( "NONE" );

namespace io
{
// *INDENT-OFF*
template<>
std::string enum_to_string<spell_target>( spell_target data )
{
    switch( data ) {
        case spell_target::ally: return "ally";
        case spell_target::hostile: return "hostile";
        case spell_target::self: return "self";
        case spell_target::ground: return "ground";
        case spell_target::none: return "none";
        case spell_target::item: return "item";
        case spell_target::fire: return "fd_fire";
        case spell_target::blood: return "fd_blood";
        case spell_target::num_spell_targets: break;
    }
    debugmsg( "Invalid valid_target" );
    abort();
}
template<>
std::string enum_to_string<body_part>( body_part data )
{
    switch( data ) {
        case body_part::bp_torso: return "TORSO";
        case body_part::bp_head: return "HEAD";
        case body_part::bp_eyes: return "EYES";
        case body_part::bp_mouth: return "MOUTH";
        case body_part::bp_arm_l: return "ARM_L";
        case body_part::bp_arm_r: return "ARM_R";
        case body_part::bp_hand_l: return "HAND_L";
        case body_part::bp_hand_r: return "HAND_R";
        case body_part::bp_leg_l: return "LEG_L";
        case body_part::bp_leg_r: return "LEG_R";
        case body_part::bp_foot_l: return "FOOT_L";
        case body_part::bp_foot_r: return "FOOT_R";
        case body_part::num_bp: break;
    }
    debugmsg( "Invalid body_part" );
    abort();
}
template<>
std::string enum_to_string<spell_flag>( spell_flag data )
{
    switch( data ) {
        case spell_flag::PERMANENT: return "PERMANENT";
        case spell_flag::IGNORE_WALLS: return "IGNORE_WALLS";
        case spell_flag::HOSTILE_SUMMON: return "HOSTILE_SUMMON";
        case spell_flag::HOSTILE_50: return "HOSTILE_50";
        case spell_flag::FRIENDLY_POLY: return "FRIENDLY_POLY";
        case spell_flag::POLYMORPH_GROUP: return "POLYMORPH_GROUP";
        case spell_flag::SILENT: return "SILENT";
        case spell_flag::LOUD: return "LOUD";
        case spell_flag::VERBAL: return "VERBAL";
        case spell_flag::SOMATIC: return "SOMATIC";
        case spell_flag::NO_HANDS: return "NO_HANDS";
        case spell_flag::NO_LEGS: return "NO_LEGS";
        case spell_flag::UNSAFE_TELEPORT: return "UNSAFE_TELEPORT";
        case spell_flag::SWAP_POS: return "SWAP_POS";
        case spell_flag::CONCENTRATE: return "CONCENTRATE";
        case spell_flag::RANDOM_AOE: return "RANDOM_AOE";
        case spell_flag::RANDOM_DAMAGE: return "RANDOM_DAMAGE";
        case spell_flag::RANDOM_DURATION: return "RANDOM_DURATION";
        case spell_flag::RANDOM_TARGET: return "RANDOM_TARGET";
        case spell_flag::MUTATE_TRAIT: return "MUTATE_TRAIT";
        case spell_flag::PAIN_NORESIST: return "PAIN_NORESIST";
        case spell_flag::WITH_CONTAINER: return "WITH_CONTAINER";
        case spell_flag::SPAWN_GROUP: return "SPAWN_GROUP";
        case spell_flag::IGNITE_FLAMMABLE: return "IGNITE_FLAMMABLE";
        case spell_flag::NO_FAIL: return "NO_FAIL";
        case spell_flag::WONDER: return "WONDER";
        case spell_flag::LAST: break;
    }
    debugmsg( "Invalid spell_flag" );
    abort();
}
// *INDENT-ON*

} // namespace io

// LOADING
// spell_type

namespace
{
generic_factory<spell_type> spell_factory( "spell" );
} // namespace

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

void spell_type::load_spell( const JsonObject &jo, const std::string &src )
{
    spell_factory.load( jo, src );
}

static magic_energy_type energy_source_from_string( const std::string &str )
{
    if( str == "MANA" ) {
        return magic_energy_type::mana;
    } else if( str == "HP" ) {
        return magic_energy_type::hp;
    } else if( str == "BIONIC" ) {
        return magic_energy_type::bionic;
    } else if( str == "STAMINA" ) {
        return magic_energy_type::stamina;
    } else if( str == "FATIGUE" ) {
        return magic_energy_type::fatigue;
    } else if( str == "NONE" ) {
        return magic_energy_type::none;
    } else {
        debugmsg( _( "ERROR: Invalid magic_energy_type string.  Defaulting to NONE" ) );
        return magic_energy_type::none;
    }
}

static damage_type damage_type_from_string( const std::string &str )
{
    if( str == "fire" ) {
        return DT_HEAT;
    } else if( str == "acid" ) {
        return DT_ACID;
    } else if( str == "bash" ) {
        return DT_BASH;
    } else if( str == "bio" ) {
        return DT_BIOLOGICAL;
    } else if( str == "cold" ) {
        return DT_COLD;
    } else if( str == "cut" ) {
        return DT_CUT;
    } else if( str == "bullet" ) {
        return DT_BULLET;
    } else if( str == "electric" ) {
        return DT_ELECTRIC;
    } else if( str == "stab" ) {
        return DT_STAB;
    } else if( str == "none" || str == "NONE" ) {
        return DT_TRUE;
    } else {
        debugmsg( _( "ERROR: Invalid damage type string.  Defaulting to none" ) );
        return DT_TRUE;
    }
}

static std::string moves_to_string( const int moves )
{
    if( moves < to_moves<int>( 2_seconds ) ) {
        return string_format( _( "%d moves" ), moves );
    } else {
        return to_string( time_duration::from_turns( moves / 100 ) );
    }
}

void spell_type::load( const JsonObject &jo, const std::string & )
{
    static const
    std::map<std::string, std::function<void( const spell &, Creature &, const tripoint & )>>
    effect_map{
        { "pain_split", spell_effect::pain_split },
        { "target_attack", spell_effect::target_attack },
        { "targeted_polymorph", spell_effect::targeted_polymorph },
        { "projectile_attack", spell_effect::projectile_attack },
        { "cone_attack", spell_effect::cone_attack },
        { "line_attack", spell_effect::line_attack },
        { "teleport_random", spell_effect::teleport_random },
        { "spawn_item", spell_effect::spawn_ethereal_item },
        { "recover_energy", spell_effect::recover_energy },
        { "summon", spell_effect::spawn_summoned_monster },
        { "summon_vehicle", spell_effect::spawn_summoned_vehicle },
        { "translocate", spell_effect::translocate },
        { "area_pull", spell_effect::area_pull },
        { "area_push", spell_effect::area_push },
        { "timed_event", spell_effect::timed_event },
        { "ter_transform", spell_effect::transform_blast },
        { "noise", spell_effect::noise },
        { "vomit", spell_effect::vomit },
        { "explosion", spell_effect::explosion },
        { "flashbang", spell_effect::flashbang },
        { "mod_moves", spell_effect::mod_moves },
        { "map", spell_effect::map },
        { "morale", spell_effect::morale },
        { "charm_monster", spell_effect::charm_monster },
        { "mutate", spell_effect::mutate },
        { "bash", spell_effect::bash },
        { "none", spell_effect::none }
    };

    mandatory( jo, was_loaded, "id", id );
    mandatory( jo, was_loaded, "name", name );
    mandatory( jo, was_loaded, "description", description );
    optional( jo, was_loaded, "skill", skill, skill_id( "spellcraft" ) );
    optional( jo, was_loaded, "components", spell_components );
    optional( jo, was_loaded, "message", message, to_translation( "You cast %s!" ) );
    optional( jo, was_loaded, "sound_description", sound_description,
              to_translation( "an explosion" ) );
    optional( jo, was_loaded, "sound_type", sound_type, sounds::sound_t::combat );
    optional( jo, was_loaded, "sound_ambient", sound_ambient, false );
    optional( jo, was_loaded, "sound_id", sound_id, "" );
    optional( jo, was_loaded, "sound_variant", sound_variant, "default" );
    mandatory( jo, was_loaded, "effect", effect_name );
    const auto found_effect = effect_map.find( effect_name );
    if( found_effect == effect_map.cend() ) {
        effect = spell_effect::none;
        debugmsg( "ERROR: spell %s has invalid effect %s", id.c_str(), effect_name );
    } else {
        effect = found_effect->second;
    }

    const auto effect_targets_reader = enum_flags_reader<spell_target> { "effect_targets" };
    optional( jo, was_loaded, "effect_filter", effect_targets, effect_targets_reader );

    const auto targeted_monster_ids_reader = auto_flags_reader<mtype_id> {};
    optional( jo, was_loaded, "targeted_monster_ids", targeted_monster_ids,
              targeted_monster_ids_reader );

    const auto trigger_reader = enum_flags_reader<spell_target> { "valid_targets" };
    mandatory( jo, was_loaded, "valid_targets", valid_targets, trigger_reader );

    if( jo.has_array( "extra_effects" ) ) {
        for( JsonObject fake_spell_obj : jo.get_array( "extra_effects" ) ) {
            fake_spell temp;
            temp.load( fake_spell_obj );
            additional_spells.emplace_back( temp );
        }
    }

    const auto bp_reader = enum_flags_reader<body_part> { "affected_bps" };
    optional( jo, was_loaded, "affected_body_parts", affected_bps, bp_reader );
    const auto flag_reader = enum_flags_reader<spell_flag> { "flags" };
    optional( jo, was_loaded, "flags", spell_tags, flag_reader );

    optional( jo, was_loaded, "effect_str", effect_str, "" );

    std::string field_input;
    optional( jo, was_loaded, "field_id", field_input, "none" );
    if( field_input != "none" ) {
        field = field_type_id( field_input );
    }
    optional( jo, was_loaded, "field_chance", field_chance, 1 );
    optional( jo, was_loaded, "min_field_intensity", min_field_intensity, 0 );
    optional( jo, was_loaded, "max_field_intensity", max_field_intensity, 0 );
    optional( jo, was_loaded, "field_intensity_increment", field_intensity_increment, 0.0f );
    optional( jo, was_loaded, "field_intensity_variance", field_intensity_variance, 0.0f );

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
    optional( jo, was_loaded, "final_energy_cost", final_energy_cost, base_energy_cost );
    optional( jo, was_loaded, "energy_increment", energy_increment, 0.0f );

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
    optional( jo, was_loaded, "final_casting_time", final_casting_time, base_casting_time );
    optional( jo, was_loaded, "casting_time_increment", casting_time_increment, 0.0f );

    for( const JsonMember member : jo.get_object( "learn_spells" ) ) {
        learn_spells.insert( std::pair<std::string, int>( member.name(), member.get_int() ) );
    }
}

static bool spell_infinite_loop_check( std::set<spell_id> spell_effects, const spell_id &sp )
{
    if( spell_effects.count( sp ) ) {
        return true;
    }
    spell_effects.emplace( sp );

    std::set<spell_id> unique_spell_list;
    for( const fake_spell &fake_sp : sp->additional_spells ) {
        unique_spell_list.emplace( fake_sp.id );
    }

    for( const spell_id &other_sp : unique_spell_list ) {
        if( spell_infinite_loop_check( spell_effects, other_sp ) ) {
            return true;
        }
    }
    return false;
}

void spell_type::check_consistency()
{
    for( const spell_type &sp_t : get_all() ) {
        if( ( sp_t.min_aoe > sp_t.max_aoe && sp_t.aoe_increment > 0 ) ||
            ( sp_t.min_aoe < sp_t.max_aoe && sp_t.aoe_increment < 0 ) ) {
            debugmsg( "ERROR: %s has higher min_aoe than max_aoe", sp_t.id.c_str() );
        }
        if( ( sp_t.min_damage > sp_t.max_damage && sp_t.damage_increment > 0 ) ||
            ( sp_t.min_damage < sp_t.max_damage && sp_t.damage_increment < 0 ) ) {
            debugmsg( "ERROR: %s has higher min_damage than max_damage", sp_t.id.c_str() );
        }
        if( ( sp_t.min_range > sp_t.max_range && sp_t.range_increment > 0 ) ||
            ( sp_t.min_range < sp_t.max_range && sp_t.range_increment < 0 ) ) {
            debugmsg( "ERROR: %s has higher min_range than max_range", sp_t.id.c_str() );
        }
        if( ( sp_t.min_dot > sp_t.max_dot && sp_t.dot_increment > 0 ) ||
            ( sp_t.min_dot < sp_t.max_dot && sp_t.dot_increment < 0 ) ) {
            debugmsg( "ERROR: %s has higher min_dot than max_dot", sp_t.id.c_str() );
        }
        if( ( sp_t.min_duration > sp_t.max_duration && sp_t.duration_increment > 0 ) ||
            ( sp_t.min_duration < sp_t.max_duration && sp_t.duration_increment < 0 ) ) {
            debugmsg( "ERROR: %s has higher min_dot_time than max_dot_time", sp_t.id.c_str() );
        }
        if( ( sp_t.min_pierce > sp_t.max_pierce && sp_t.pierce_increment > 0 ) ||
            ( sp_t.min_pierce < sp_t.max_pierce && sp_t.pierce_increment < 0 ) ) {
            debugmsg( "ERROR: %s has higher min_pierce than max_pierce", sp_t.id.c_str() );
        }
        if( sp_t.casting_time_increment < 0.0f && sp_t.base_casting_time < sp_t.final_casting_time ) {
            debugmsg( "ERROR: %s has negative increment and base_casting_time < final_casting_time",
                      sp_t.id.c_str() );
        }
        if( sp_t.casting_time_increment > 0.0f && sp_t.base_casting_time > sp_t.final_casting_time ) {
            debugmsg( "ERROR: %s has positive increment and base_casting_time > final_casting_time",
                      sp_t.id.c_str() );
        }
        if( sp_t.effect_name == "summon_vehicle" ) {
            if( !sp_t.effect_str.empty() && !vproto_id( sp_t.effect_str ).is_valid() ) {
                debugmsg( "ERROR %s specifies a vehicle to summon, but vehicle %s is not valid", sp_t.id.c_str(),
                          sp_t.effect_str );
            }
        }
        std::set<spell_id> spell_effect_list;
        if( spell_infinite_loop_check( spell_effect_list, sp_t.id ) ) {
            debugmsg( "ERROR: %s has infinite loop in extra_effects", sp_t.id.c_str() );
        }
        if( sp_t.field ) {
            if( sp_t.field_chance <= 0 ) {
                debugmsg( "ERROR: %s must have a positive field chance.", sp_t.id.c_str() );
            }
            if( sp_t.field_intensity_increment > 0 && sp_t.max_field_intensity < sp_t.min_field_intensity ) {
                debugmsg( "ERROR: max_field_intensity must be greater than min_field_intensity with positive increment: %s",
                          sp_t.id.c_str() );
            } else if( sp_t.field_intensity_increment < 0 &&
                       sp_t.max_field_intensity > sp_t.min_field_intensity ) {
                debugmsg( "ERROR: min_field_intensity must be greater than max_field_intensity with negative increment: %s",
                          sp_t.id.c_str() );
            }
        }
        if( sp_t.spell_tags[spell_flag::WONDER] && sp_t.additional_spells.empty() ) {
            debugmsg( "ERROR: %s has WONDER flag but no spells to choose from!", sp_t.id.c_str() );
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

spell::spell( spell_id sp, int xp ) :
    type( sp ),
    experience( xp )
{}

void spell::set_message( const translation &msg )
{
    alt_message = msg;
}

spell_id spell::id() const
{
    return type;
}

trait_id spell::spell_class() const
{
    return type->spell_class;
}

skill_id spell::skill() const
{
    return type->skill;
}

int spell::field_intensity() const
{
    return std::min( type->max_field_intensity,
                     static_cast<int>( type->min_field_intensity + std::round( get_level() *
                                       type->field_intensity_increment ) ) );
}

int spell::min_leveled_damage() const
{
    return type->min_damage + std::round( get_level() * type->damage_increment );
}

int spell::damage() const
{
    const int leveled_damage = min_leveled_damage();

    if( has_flag( spell_flag::RANDOM_DAMAGE ) ) {
        return rng( std::min( leveled_damage, type->max_damage ), std::max( leveled_damage,
                    type->max_damage ) );
    } else {
        if( type->min_damage >= 0 || type->max_damage >= type->min_damage ) {
            return std::min( leveled_damage, type->max_damage );
        } else { // if it's negative, min and max work differently
            return std::max( leveled_damage, type->max_damage );
        }
    }
}

int spell::min_leveled_dot() const
{
    return type->min_dot + std::round( get_level() * type->dot_increment );
}

int spell::damage_dot() const
{
    const int leveled_dot = min_leveled_dot();
    if( type->min_dot >= 0 || type->max_dot >= type->min_dot ) {
        return std::min( leveled_dot, type->max_dot );
    } else { // if it's negative, min and max work differently
        return std::max( leveled_dot, type->max_dot );
    }
}

damage_over_time_data spell::damage_over_time( const std::vector<bodypart_str_id> &bps ) const
{
    damage_over_time_data temp;
    temp.bps = bps;
    temp.duration = duration_turns();
    temp.amount = damage_dot();
    temp.type = dmg_type();
    return temp;
}

std::string spell::damage_string() const
{
    if( has_flag( spell_flag::RANDOM_DAMAGE ) ) {
        return string_format( "%d-%d", min_leveled_damage(), type->max_damage );
    } else {
        const int dmg = damage();
        if( dmg >= 0 ) {
            return string_format( "%d", dmg );
        } else {
            return string_format( "+%d", std::abs( dmg ) );
        }
    }
}

int spell::min_leveled_aoe() const
{
    return type->min_aoe + std::round( get_level() * type->aoe_increment );
}

int spell::aoe() const
{
    const int leveled_aoe = min_leveled_aoe();

    if( has_flag( spell_flag::RANDOM_AOE ) ) {
        return rng( std::min( leveled_aoe, type->max_aoe ), std::max( leveled_aoe, type->max_aoe ) );
    } else {
        if( type->max_aoe >= type->min_aoe ) {
            return std::min( leveled_aoe, type->max_aoe );
        } else {
            return std::max( leveled_aoe, type->max_aoe );
        }
    }
}

bool spell::in_aoe( const tripoint &source, const tripoint &target ) const
{
    if( has_flag( spell_flag::RANDOM_AOE ) ) {
        return rl_dist( source, target ) <= type->max_aoe;
    } else {
        return rl_dist( source, target ) <= aoe();
    }
}

std::string spell::aoe_string() const
{
    if( has_flag( spell_flag::RANDOM_AOE ) ) {
        return string_format( "%d-%d", min_leveled_aoe(), type->max_aoe );
    } else {
        return string_format( "%d", aoe() );
    }
}

int spell::range() const
{
    const int leveled_range = type->min_range + std::round( get_level() * type->range_increment );
    if( type->max_range >= type->min_range ) {
        return std::min( leveled_range, type->max_range );
    } else {
        return std::max( leveled_range, type->max_range );
    }
}

int spell::min_leveled_duration() const
{
    return type->min_duration + std::round( get_level() * type->duration_increment );
}

int spell::duration() const
{
    const int leveled_duration = min_leveled_duration();

    if( has_flag( spell_flag::RANDOM_DURATION ) ) {
        return rng( std::min( leveled_duration, type->max_duration ), std::max( leveled_duration,
                    type->max_duration ) );
    } else {
        if( type->max_duration >= type->min_duration ) {
            return std::min( leveled_duration, type->max_duration );
        } else {
            return std::max( leveled_duration, type->max_duration );
        }
    }
}

std::string spell::duration_string() const
{
    if( has_flag( spell_flag::RANDOM_DURATION ) ) {
        return string_format( "%s - %s", moves_to_string( min_leveled_duration() ),
                              moves_to_string( type->max_duration ) );
    } else {
        return moves_to_string( duration() );
    }
}

time_duration spell::duration_turns() const
{
    return 1_turns * duration() / 100;
}

void spell::gain_level()
{
    gain_exp( exp_to_next_level() );
}

void spell::gain_levels( int gains )
{
    if( gains < 1 ) {
        return;
    }
    for( int gained = 0; gained < gains && !is_max_level(); gained++ ) {
        gain_level();
    }
}

void spell::set_level( int nlevel )
{
    experience = 0;
    gain_levels( nlevel );
}

bool spell::is_max_level() const
{
    return get_level() >= type->max_level;
}

bool spell::can_learn( const Character &guy ) const
{
    if( type->spell_class == trait_NONE ) {
        return true;
    }
    return guy.has_trait( type->spell_class );
}

int spell::energy_cost( const Character &guy ) const
{
    int cost;
    if( type->base_energy_cost < type->final_energy_cost ) {
        cost = std::min( type->final_energy_cost,
                         static_cast<int>( std::round( type->base_energy_cost + type->energy_increment * get_level() ) ) );
    } else if( type->base_energy_cost > type->final_energy_cost ) {
        cost = std::max( type->final_energy_cost,
                         static_cast<int>( std::round( type->base_energy_cost + type->energy_increment * get_level() ) ) );
    } else {
        cost = type->base_energy_cost;
    }
    if( !has_flag( spell_flag::NO_HANDS ) ) {
        // the first 10 points of combined encumbrance is ignored, but quickly adds up
        const int hands_encumb = std::max( 0,
                                           guy.encumb( bodypart_id( "hand_l" ) ) + guy.encumb( bodypart_id( "hand_r" ) ) - 10 );
        switch( type->energy_source ) {
            default:
                cost += 10 * hands_encumb;
                break;
            case magic_energy_type::hp:
                cost += hands_encumb;
                break;
            case magic_energy_type::stamina:
                cost += 100 * hands_encumb;
                break;
        }
    }
    return cost;
}

bool spell::has_flag( const spell_flag &flag ) const
{
    return type->spell_tags[flag];
}

bool spell::is_spell_class( const trait_id &mid ) const
{
    return mid == type->spell_class;
}

bool spell::can_cast( Character &guy ) const
{
    if( !type->spell_components.is_empty() &&
        !type->spell_components->can_make_with_inventory( guy.crafting_inventory( guy.pos(), 0 ),
                return_true<item> ) ) {
        return false;
    }

    switch( type->energy_source ) {
        case magic_energy_type::mana:
            return guy.magic->available_mana() >= energy_cost( guy );
        case magic_energy_type::stamina:
            return guy.get_stamina() >= energy_cost( guy );
        case magic_energy_type::hp: {
            for( const std::pair<const bodypart_str_id, bodypart> &elem : guy.get_body() ) {
                if( energy_cost( guy ) < elem.second.get_hp_cur() ) {
                    return true;
                }
            }
            return false;
        }
        case magic_energy_type::bionic:
            return guy.get_power_level() >= units::from_kilojoule( energy_cost( guy ) );
        case magic_energy_type::fatigue:
            return guy.get_fatigue() < fatigue_levels::EXHAUSTED;
        case magic_energy_type::none:
        default:
            return true;
    }
}

void spell::use_components( Character &guy ) const
{
    if( type->spell_components.is_empty() ) {
        return;
    }
    const requirement_data &spell_components = type->spell_components.obj();
    // if we're here, we're assuming the Character has the correct components (using can_cast())
    inventory map_inv;
    for( const std::vector<item_comp> &comp_vec : spell_components.get_components() ) {
        guy.consume_items( guy.select_item_component( comp_vec, 1, map_inv ), 1 );
    }
    for( const std::vector<tool_comp> &tool_vec : spell_components.get_tools() ) {
        guy.consume_tools( guy.select_tool_component( tool_vec, 1, map_inv ), 1 );
    }
}

int spell::get_difficulty() const
{
    return type->difficulty;
}

int spell::casting_time( const Character &guy, bool ignore_encumb ) const
{
    // casting time in moves
    int casting_time = 0;
    if( type->base_casting_time < type->final_casting_time ) {
        casting_time = std::min( type->final_casting_time,
                                 static_cast<int>( std::round( type->base_casting_time + type->casting_time_increment *
                                         get_level() ) ) );
    } else if( type->base_casting_time > type->final_casting_time ) {
        casting_time = std::max( type->final_casting_time,
                                 static_cast<int>( std::round( type->base_casting_time + type->casting_time_increment *
                                         get_level() ) ) );
    } else {
        casting_time = type->base_casting_time;
    }
    if( !ignore_encumb ) {
        if( !has_flag( spell_flag::NO_LEGS ) ) {
            // the first 20 points of encumbrance combined is ignored
            const int legs_encumb = std::max( 0,
                                              guy.encumb( bodypart_id( "leg_l" ) ) + guy.encumb( bodypart_id( "leg_r" ) ) - 20 );
            casting_time += legs_encumb * 3;
        }
        if( has_flag( spell_flag::SOMATIC ) ) {
            // the first 20 points of encumbrance combined is ignored
            const int arms_encumb = std::max( 0,
                                              guy.encumb( bodypart_id( "arm_l" ) ) + guy.encumb( bodypart_id( "arm_r" ) ) - 20 );
            casting_time += arms_encumb * 2;
        }
    }
    return casting_time;
}

const requirement_data &spell::components() const
{
    return type->spell_components.obj();
}

bool spell::has_components() const
{
    return !type->spell_components.is_empty();
}

std::string spell::name() const
{
    return type->name.translated();
}

std::string spell::message() const
{
    if( !alt_message.empty() ) {
        return alt_message.translated();
    }
    return type->message.translated();
}

float spell::spell_fail( const Character &guy ) const
{
    if( has_flag( spell_flag::NO_FAIL ) ) {
        return 0.0f;
    }
    // formula is based on the following:
    // exponential curve
    // effective skill of 0 or less is 100% failure
    // effective skill of 8 (8 int, 0 spellcraft, 0 spell level, spell difficulty 0) is ~50% failure
    // effective skill of 30 is 0% failure
    const float effective_skill = 2 * ( get_level() - get_difficulty() ) + guy.get_int() +
                                  guy.get_skill_level( skill() );
    // add an if statement in here because sufficiently large numbers will definitely overflow because of exponents
    if( effective_skill > 30.0f ) {
        return 0.0f;
    } else if( effective_skill < 0.0f ) {
        return 1.0f;
    }
    float fail_chance = std::pow( ( effective_skill - 30.0f ) / 30.0f, 2 );
    if( has_flag( spell_flag::SOMATIC ) && !guy.has_trait_flag( "SUBTLE_SPELL" ) ) {
        // the first 20 points of encumbrance combined is ignored
        const int arms_encumb = std::max( 0,
                                          guy.encumb( bodypart_id( "arm_l" ) ) + guy.encumb( bodypart_id( "arm_r" ) ) - 20 );
        // each encumbrance point beyond the "gray" color counts as half an additional fail %
        fail_chance += arms_encumb / 200.0f;
    }
    if( has_flag( spell_flag::VERBAL ) && !guy.has_trait_flag( "SILENT_SPELL" ) ) {
        // a little bit of mouth encumbrance is allowed, but not much
        const int mouth_encumb = std::max( 0, guy.encumb( bodypart_id( "mouth" ) ) - 5 );
        fail_chance += mouth_encumb / 100.0f;
    }
    // concentration spells work better than you'd expect with a higher focus pool
    if( has_flag( spell_flag::CONCENTRATE ) ) {
        if( guy.focus_pool <= 0 ) {
            return 0.0f;
        }
        fail_chance /= guy.focus_pool / 100.0f;
    }
    return clamp( fail_chance, 0.0f, 1.0f );
}

std::string spell::colorized_fail_percent( const Character &guy ) const
{
    const float fail_fl = spell_fail( guy ) * 100.0f;
    std::string fail_str;
    fail_fl == 100.0f ? fail_str = _( "Too Difficult!" ) : fail_str = string_format( "%.1f %% %s",
                                   fail_fl, _( "Failure Chance" ) );
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

void spell::set_exp( int nxp )
{
    experience = nxp;
}

std::string spell::energy_string() const
{
    switch( type->energy_source ) {
        case magic_energy_type::hp:
            return _( "health" );
        case magic_energy_type::mana:
            return _( "mana" );
        case magic_energy_type::stamina:
            return _( "stamina" );
        case magic_energy_type::bionic:
            return _( "bionic power" );
        case magic_energy_type::fatigue:
            return _( "fatigue" );
        default:
            return "";
    }
}

std::string spell::energy_cost_string( const Character &guy ) const
{
    if( energy_source() == magic_energy_type::none ) {
        return _( "none" );
    }
    if( energy_source() == magic_energy_type::bionic || energy_source() == magic_energy_type::mana ) {
        return colorize( to_string( energy_cost( guy ) ), c_light_blue );
    }
    if( energy_source() == magic_energy_type::hp ) {
        auto pair = get_hp_bar( energy_cost( guy ), guy.get_hp_max() / 6 );
        return colorize( pair.first, pair.second );
    }
    if( energy_source() == magic_energy_type::stamina ) {
        auto pair = get_hp_bar( energy_cost( guy ), guy.get_stamina_max() );
        return colorize( pair.first, pair.second );
    }
    if( energy_source() == magic_energy_type::fatigue ) {
        return colorize( to_string( energy_cost( guy ) ), c_cyan );
    }
    debugmsg( "ERROR: Spell %s has invalid energy source.", id().c_str() );
    return _( "error: energy_type" );
}

std::string spell::energy_cur_string( const Character &guy ) const
{
    if( energy_source() == magic_energy_type::none ) {
        return _( "infinite" );
    }
    if( energy_source() == magic_energy_type::bionic ) {
        return colorize( to_string( units::to_kilojoule( guy.get_power_level() ) ), c_light_blue );
    }
    if( energy_source() == magic_energy_type::mana ) {
        return colorize( to_string( guy.magic->available_mana() ), c_light_blue );
    }
    if( energy_source() == magic_energy_type::stamina ) {
        auto pair = get_hp_bar( guy.get_stamina(), guy.get_stamina_max() );
        return colorize( pair.first, pair.second );
    }
    if( energy_source() == magic_energy_type::hp ) {
        return "";
    }
    if( energy_source() == magic_energy_type::fatigue ) {
        const std::pair<std::string, nc_color> pair = guy.get_fatigue_description();
        return colorize( pair.first, pair.second );
    }
    debugmsg( "ERROR: Spell %s has invalid energy source.", id().c_str() );
    return _( "error: energy_type" );
}

bool spell::is_valid() const
{
    return type.is_valid();
}

bool spell::bp_is_affected( body_part bp ) const
{
    return type->affected_bps[bp];
}

void spell::create_field( const tripoint &at ) const
{
    if( !type->field ) {
        return;
    }
    const int intensity = field_intensity() + rng( -type->field_intensity_variance * field_intensity(),
                          type->field_intensity_variance * field_intensity() );
    if( intensity <= 0 ) {
        return;
    }
    if( one_in( type->field_chance ) ) {
        map &here = get_map();
        field_entry *field = here.get_field( at, *type->field );
        if( field ) {
            field->set_field_intensity( field->get_field_intensity() + intensity );
        } else {
            here.add_field( at, *type->field, intensity, -duration_turns() );
        }
    }
}

void spell::make_sound( const tripoint &target ) const
{
    if( !has_flag( spell_flag::SILENT ) ) {
        int loudness = std::abs( damage() ) / 3;
        if( has_flag( spell_flag::LOUD ) ) {
            loudness += 1 + damage() / 3;
        }
        make_sound( target, loudness );
    }
}

void spell::make_sound( const tripoint &target, int loudness ) const
{
    sounds::sound( target, loudness, type->sound_type, type->sound_description.translated(),
                   type->sound_ambient, type->sound_id, type->sound_variant );
}

std::string spell::effect() const
{
    return type->effect_name;
}

magic_energy_type spell::energy_source() const
{
    return type->energy_source;
}

bool spell::is_target_in_range( const Creature &caster, const tripoint &p ) const
{
    return rl_dist( caster.pos(), p ) <= range();
}

bool spell::is_valid_target( spell_target t ) const
{
    return type->valid_targets[t];
}

bool spell::is_valid_target( const Creature &caster, const tripoint &p ) const
{
    bool valid = false;
    if( Creature *const cr = g->critter_at<Creature>( p ) ) {
        Creature::Attitude cr_att = cr->attitude_to( caster );
        valid = valid || ( cr_att != Creature::Attitude::FRIENDLY &&
                           is_valid_target( spell_target::hostile ) );
        valid = valid || ( cr_att == Creature::Attitude::FRIENDLY &&
                           is_valid_target( spell_target::ally ) &&
                           p != caster.pos() );
        valid = valid || ( is_valid_target( spell_target::self ) && p == caster.pos() );
        valid = valid && target_by_monster_id( p );
    } else {
        valid = is_valid_target( spell_target::ground );
    }
    return valid;
}

bool spell::is_valid_effect_target( spell_target t ) const
{
    return type->effect_targets[t];
}

bool spell::target_by_monster_id( const tripoint &p ) const
{
    if( type->targeted_monster_ids.empty() ) {
        return true;
    }
    bool valid = false;
    if( monster *const target = g->critter_at<monster>( p ) ) {
        if( type->targeted_monster_ids.find( target->type->id ) != type->targeted_monster_ids.end() ) {
            valid = true;
        }
    }
    return valid;
}

std::string spell::description() const
{
    return type->description.translated();
}

nc_color spell::damage_type_color() const
{
    switch( dmg_type() ) {
        case DT_HEAT:
            return c_red;
        case DT_ACID:
            return c_light_green;
        case DT_BASH:
            return c_magenta;
        case DT_BIOLOGICAL:
            return c_green;
        case DT_COLD:
            return c_white;
        case DT_CUT:
            return c_light_gray;
        case DT_ELECTRIC:
            return c_light_blue;
        case DT_BULLET:
        /* fallthrough */
        case DT_STAB:
            return c_light_red;
        case DT_TRUE:
            return c_dark_gray;
        default:
            return c_black;
    }
}

std::string spell::damage_type_string() const
{
    return name_by_dt( dmg_type() );
}

// constants defined below are just for the formula to be used,
// in order for the inverse formula to be equivalent
constexpr float a = 6200.0f;
constexpr float b = 0.146661f;
constexpr float c = -62.5f;

int spell::get_level() const
{
    // you aren't at the next level unless you have the requisite xp, so floor
    return std::max( static_cast<int>( std::floor( std::log( experience + a ) / b + c ) ), 0 );
}

int spell::get_max_level() const
{
    return type->max_level;
}

// helper function to calculate xp needed to be at a certain level
// pulled out as a helper function to make it easier to either be used in the future
// or easier to tweak the formula
static int exp_for_level( int level )
{
    // level 0 never needs xp
    if( level == 0 ) {
        return 0;
    }
    return std::ceil( std::exp( ( level - c ) * b ) ) - a;
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
    return string_format( "%i%%", clamp( static_cast<int>( std::round( progress * 100 ) ), 0, 99 ) );
}

float spell::exp_modifier( const Character &guy ) const
{
    const float int_modifier = ( guy.get_int() - 8.0f ) / 8.0f;
    const float difficulty_modifier = get_difficulty() / 20.0f;
    const float spellcraft_modifier = guy.get_skill_level( skill() ) / 10.0f;

    return ( int_modifier + difficulty_modifier + spellcraft_modifier ) / 5.0f + 1.0f;
}

int spell::casting_exp( const Character &guy ) const
{
    // the amount of xp you would get with no modifiers
    const int base_casting_xp = 75;

    return std::round( guy.adjust_for_focus( base_casting_xp * exp_modifier( guy ) ) );
}

std::string spell::enumerate_targets() const
{
    std::vector<std::string> all_valid_targets;
    int last_target = static_cast<int>( spell_target::num_spell_targets );
    for( int i = 0; i < last_target; ++i ) {
        spell_target t = static_cast<spell_target>( i );
        if( is_valid_target( t ) && t != spell_target::none ) {
            all_valid_targets.emplace_back( io::enum_to_string( t ) );
        }
    }
    if( all_valid_targets.size() == 1 ) {
        return all_valid_targets[0];
    }
    std::string ret;
    // TODO: if only we had a function to enumerate strings and concatenate them...
    for( auto iter = all_valid_targets.begin(); iter != all_valid_targets.end(); iter++ ) {
        if( iter + 1 == all_valid_targets.end() ) {
            ret = string_format( _( "%s and %s" ), ret, *iter );
        } else if( iter == all_valid_targets.begin() ) {
            ret = *iter;
        } else {
            ret = string_format( _( "%s, %s" ), ret, *iter );
        }
    }
    return ret;
}

std::string spell::list_targeted_monster_names() const
{
    if( type->targeted_monster_ids.empty() ) {
        return "";
    }
    std::vector<std::string> all_valid_monster_names;
    for( const mtype_id &mon_id : type->targeted_monster_ids ) {
        all_valid_monster_names.emplace_back( mon_id->nname() );
    }
    //remove repeat names
    all_valid_monster_names.erase( std::unique( all_valid_monster_names.begin(),
                                   all_valid_monster_names.end() ), all_valid_monster_names.end() );
    std::string ret = enumerate_as_string( all_valid_monster_names );
    return ret;
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
    return dmg;
}

std::string spell::effect_data() const
{
    return type->effect_str;
}

vproto_id spell::summon_vehicle_id() const
{
    return vproto_id( type->effect_str );
}

int spell::heal( const tripoint &target ) const
{
    monster *const mon = g->critter_at<monster>( target );
    if( mon ) {
        return mon->heal( -damage() );
    }
    Character *const p = g->critter_at<Character>( target );
    if( p ) {
        p->healall( -damage() );
        return -damage();
    }
    return -1;
}

void spell::cast_spell_effect( Creature &source, const tripoint &target ) const
{
    type->effect( *this, source, target );
}

void spell::cast_all_effects( Creature &source, const tripoint &target ) const
{
    if( has_flag( spell_flag::WONDER ) ) {
        const auto iter = type->additional_spells.begin();
        for( int num_spells = std::abs( damage() ); num_spells > 0; num_spells-- ) {
            if( type->additional_spells.empty() ) {
                debugmsg( "ERROR: %s has WONDER flag but no spells to choose from!", type->id.c_str() );
                return;
            }
            const int rand_spell = rng( 0, type->additional_spells.size() - 1 );
            spell sp = ( iter + rand_spell )->get_spell( get_level() );
            const bool _self = ( iter + rand_spell )->self;

            // This spell flag makes it so the message of the spell that's cast using this spell will be sent.
            // if a message is added to the casting spell, it will be sent as well.
            source.add_msg_if_player( sp.message() );

            if( sp.has_flag( spell_flag::RANDOM_TARGET ) ) {
                if( const cata::optional<tripoint> new_target = sp.random_valid_target( source,
                        _self ? source.pos() : target ) ) {
                    sp.cast_all_effects( source, *new_target );
                }
            } else {
                if( _self ) {
                    sp.cast_all_effects( source, source.pos() );
                } else {
                    sp.cast_all_effects( source, target );
                }
            }
        }
    } else {
        // first call the effect of the main spell
        cast_spell_effect( source, target );
        for( const fake_spell &extra_spell : type->additional_spells ) {
            spell sp = extra_spell.get_spell( get_level() );
            if( sp.has_flag( spell_flag::RANDOM_TARGET ) ) {
                if( const cata::optional<tripoint> new_target = sp.random_valid_target( source,
                        extra_spell.self ? source.pos() : target ) ) {
                    sp.cast_all_effects( source, *new_target );
                }
            } else {
                if( extra_spell.self ) {
                    sp.cast_all_effects( source, source.pos() );
                } else {
                    sp.cast_all_effects( source, target );
                }
            }
        }
    }
}

cata::optional<tripoint> spell::random_valid_target( const Creature &caster,
        const tripoint &caster_pos ) const
{
    std::set<tripoint> valid_area;
    for( const tripoint &target : spell_effect::spell_effect_blast( *this, caster_pos, caster_pos,
            range(), false ) ) {
        if( is_valid_target( caster, target ) ) {
            valid_area.emplace( target );
        }
    }
    if( valid_area.empty() ) {
        return cata::nullopt;
    }
    return random_entry( valid_area );
}

// player

known_magic::known_magic()
{
    mana_base = 1000;
    mana = mana_base;
}

void known_magic::serialize( JsonOut &json ) const
{
    json.start_object();

    json.member( "mana", mana );

    json.member( "spellbook" );
    json.start_array();
    for( const auto &pair : spellbook ) {
        json.start_object();
        json.member( "id", pair.second.id() );
        json.member( "xp", pair.second.xp() );
        json.end_object();
    }
    json.end_array();
    json.member( "invlets", invlets );

    json.end_object();
}

void known_magic::deserialize( JsonIn &jsin )
{
    JsonObject data = jsin.get_object();
    data.read( "mana", mana );

    for( JsonObject jo : data.get_array( "spellbook" ) ) {
        std::string id = jo.get_string( "id" );
        spell_id sp = spell_id( id );
        int xp = jo.get_int( "xp" );
        if( knows_spell( sp ) ) {
            spellbook[sp].set_exp( xp );
        } else {
            spellbook.emplace( sp, spell( sp, xp ) );
        }
    }
    data.read( "invlets", invlets );
}

bool known_magic::knows_spell( const std::string &sp ) const
{
    return knows_spell( spell_id( sp ) );
}

bool known_magic::knows_spell( const spell_id &sp ) const
{
    return spellbook.count( sp ) == 1;
}

bool known_magic::knows_spell() const
{
    return !spellbook.empty();
}

void known_magic::learn_spell( const std::string &sp, Character &guy, bool force )
{
    learn_spell( spell_id( sp ), guy, force );
}

void known_magic::learn_spell( const spell_id &sp, Character &guy, bool force )
{
    learn_spell( &sp.obj(), guy, force );
}

void known_magic::learn_spell( const spell_type *sp, Character &guy, bool force )
{
    if( !sp->is_valid() ) {
        debugmsg( "Tried to learn invalid spell" );
        return;
    }
    if( guy.magic->knows_spell( sp->id ) ) {
        // you already know the spell
        return;
    }
    spell temp_spell( sp->id );
    if( !temp_spell.is_valid() ) {
        debugmsg( "Tried to learn invalid spell" );
        return;
    }
    if( !force && sp->spell_class != trait_NONE ) {
        if( can_learn_spell( guy, sp->id ) && !guy.has_trait( sp->spell_class ) ) {
            std::string trait_cancel;
            for( const trait_id &cancel : sp->spell_class->cancels ) {
                if( cancel == sp->spell_class->cancels.back() &&
                    sp->spell_class->cancels.back() != sp->spell_class->cancels.front() ) {
                    trait_cancel = string_format( _( "%s and %s" ), trait_cancel, cancel->name() );
                } else if( cancel == sp->spell_class->cancels.front() ) {
                    trait_cancel = cancel->name();
                    if( sp->spell_class->cancels.size() == 1 ) {
                        trait_cancel = string_format( "%s: %s", trait_cancel, cancel->desc() );
                    }
                } else {
                    trait_cancel = string_format( _( "%s, %s" ), trait_cancel, cancel->name() );
                }
                if( cancel == sp->spell_class->cancels.back() ) {
                    trait_cancel += ".";
                }
            }
            if( query_yn(
                    _( "Learning this spell will make you a\n\n%s: %s\n\nand lock you out of\n\n%s\n\nContinue?" ),
                    sp->spell_class->name(), sp->spell_class->desc(), trait_cancel ) ) {
                guy.set_mutation( sp->spell_class );
                guy.on_mutation_gain( sp->spell_class );
                guy.add_msg_if_player( sp->spell_class.obj().desc() );
            } else {
                return;
            }
        }
    }
    if( force || can_learn_spell( guy, sp->id ) ) {
        spellbook.emplace( sp->id, temp_spell );
        get_event_bus().send<event_type::character_learns_spell>( guy.getID(), sp->id );
        guy.add_msg_if_player( m_good, _( "You learned %s!" ), sp->name );
    } else {
        guy.add_msg_if_player( m_bad, _( "You can't learn this spell." ) );
    }
}

void known_magic::forget_spell( const std::string &sp )
{
    forget_spell( spell_id( sp ) );
}

void known_magic::forget_spell( const spell_id &sp )
{
    if( !knows_spell( sp ) ) {
        debugmsg( "Can't forget a spell you don't know!" );
        return;
    }
    add_msg( m_bad, _( "All knowledge of %s leaves you." ), sp->name );
    // TODO: add parameter for owner of known_magic for this function
    get_event_bus().send<event_type::character_forgets_spell>( get_player_character().getID(), sp->id );
    spellbook.erase( sp );
}

bool known_magic::can_learn_spell( const Character &guy, const spell_id &sp ) const
{
    const spell_type &sp_t = sp.obj();
    if( sp_t.spell_class == trait_NONE ) {
        return true;
    }
    return !guy.has_opposite_trait( sp_t.spell_class );
}

spell &known_magic::get_spell( const spell_id &sp )
{
    if( !knows_spell( sp ) ) {
        debugmsg( "ERROR: Tried to get unknown spell" );
    }
    spell &temp_spell = spellbook[ sp ];
    return temp_spell;
}

std::vector<spell *> known_magic::get_spells()
{
    std::vector<spell *> spells;
    for( auto &spell_pair : spellbook ) {
        spells.emplace_back( &spell_pair.second );
    }
    return spells;
}

int known_magic::available_mana() const
{
    return mana;
}

void known_magic::set_mana( int new_mana )
{
    mana = new_mana;
}

void known_magic::mod_mana( const Character &guy, int add_mana )
{
    set_mana( clamp( mana + add_mana, 0, max_mana( guy ) ) );
}

int known_magic::max_mana( const Character &guy ) const
{
    const float int_bonus = ( ( 0.2f + guy.get_int() * 0.1f ) - 1.0f ) * mana_base;
    const float unaugmented_mana = std::max( 0.0f,
                                   ( ( mana_base + int_bonus ) * guy.mutation_value( "mana_multiplier" ) ) +
                                   guy.mutation_value( "mana_modifier" ) - units::to_kilojoule( guy.get_power_level() ) );
    return guy.calculate_by_enchantment( unaugmented_mana, enchant_vals::mod::MAX_MANA, true );
}

void known_magic::update_mana( const Character &guy, float turns )
{
    // mana should replenish in 8 hours.
    const float full_replenish = to_turns<float>( 8_hours );
    const float ratio = turns / full_replenish;
    mod_mana( guy, std::floor( ratio * guy.calculate_by_enchantment( max_mana( guy ) *
                               guy.mutation_value( "mana_regen_multiplier" ), enchant_vals::mod::REGEN_MANA ) ) );
}

std::vector<spell_id> known_magic::spells() const
{
    std::vector<spell_id> spell_ids;
    for( const std::pair<const spell_id, spell> &pair : spellbook ) {
        spell_ids.emplace_back( pair.first );
    }
    return spell_ids;
}

// does the Character have enough energy (of the type of the spell) to cast the spell?
bool known_magic::has_enough_energy( const Character &guy, spell &sp ) const
{
    int cost = sp.energy_cost( guy );
    switch( sp.energy_source() ) {
        case magic_energy_type::mana:
            return available_mana() >= cost;
        case magic_energy_type::bionic:
            return guy.get_power_level() >= units::from_kilojoule( cost );
        case magic_energy_type::stamina:
            return guy.get_stamina() >= cost;
        case magic_energy_type::hp:
            for( const std::pair<const bodypart_str_id, bodypart> &elem : guy.get_body() ) {
                if( elem.second.get_hp_cur() > cost ) {
                    return true;
                }
            }
            return false;
        case magic_energy_type::fatigue:
            return guy.get_fatigue() < fatigue_levels::EXHAUSTED;
        case magic_energy_type::none:
            return true;
        default:
            return false;
    }
}

int known_magic::time_to_learn_spell( const Character &guy, const std::string &str ) const
{
    return time_to_learn_spell( guy, spell_id( str ) );
}

int known_magic::time_to_learn_spell( const Character &guy, const spell_id &sp ) const
{
    const int base_time = to_moves<int>( 30_minutes );
    return base_time * ( 1.0 + sp->difficulty / ( 1.0 + ( guy.get_int() - 8.0 ) / 8.0 ) +
                         ( guy.get_skill_level( sp->skill ) / 10.0 ) );
}

int known_magic::get_spellname_max_width()
{
    int width = 0;
    for( const spell *sp : get_spells() ) {
        width = std::max( width, utf8_width( sp->name() ) );
    }
    return width;
}

class spellcasting_callback : public uilist_callback
{
    private:
        std::vector<spell *> known_spells;
        void draw_spell_info( const spell &sp, const uilist *menu );
    public:
        // invlets reserved for special functions
        const std::set<int> reserved_invlets{ 'I', '=' };
        bool casting_ignore;

        spellcasting_callback( std::vector<spell *> &spells,
                               bool casting_ignore ) : known_spells( spells ),
            casting_ignore( casting_ignore ) {}
        bool key( const input_context &, const input_event &event, int entnum,
                  uilist * /*menu*/ ) override {
            if( event.get_first_input() == 'I' ) {
                casting_ignore = !casting_ignore;
                return true;
            }
            if( event.get_first_input() == '=' ) {
                int invlet = 0;
                invlet = popup_getkey( _( "Choose a new hotkey for this spell." ) );
                if( inv_chars.valid( invlet ) ) {
                    const bool invlet_set =
                        get_player_character().magic->set_invlet( known_spells[entnum]->id(), invlet, reserved_invlets );
                    if( !invlet_set ) {
                        popup( _( "Hotkey already used." ) );
                    } else {
                        popup( _( "%c set.  Close and reopen spell menu to refresh list with changes." ),
                               invlet );
                    }
                } else {
                    popup( _( "Hotkey removed." ) );
                    get_player_character().magic->rem_invlet( known_spells[entnum]->id() );
                }
                return true;
            }
            return false;
        }

        void refresh( uilist *menu ) override {
            mvwputch( menu->window, point( menu->w_width - menu->pad_right, 0 ), c_magenta, LINE_OXXX );
            mvwputch( menu->window, point( menu->w_width - menu->pad_right, menu->w_height - 1 ), c_magenta,
                      LINE_XXOX );
            for( int i = 1; i < menu->w_height - 1; i++ ) {
                mvwputch( menu->window, point( menu->w_width - menu->pad_right, i ), c_magenta, LINE_XOXO );
            }
            std::string ignore_string = casting_ignore ? _( "Ignore Distractions" ) :
                                        _( "Popup Distractions" );
            mvwprintz( menu->window, point( menu->w_width - menu->pad_right + 2, 0 ),
                       casting_ignore ? c_red : c_light_green, string_format( "%s %s", "[I]", ignore_string ) );
            const std::string assign_letter = _( "Assign Hotkey [=]" );
            mvwprintz( menu->window, point( menu->w_width - assign_letter.length() - 1, 0 ), c_yellow,
                       assign_letter );
            if( menu->selected >= 0 && static_cast<size_t>( menu->selected ) < known_spells.size() ) {
                draw_spell_info( *known_spells[menu->selected], menu );
            }
            wnoutrefresh( menu->window );
        }
};

static bool casting_time_encumbered( const spell &sp, const Character &guy )
{
    int encumb = 0;
    if( !sp.has_flag( spell_flag::NO_LEGS ) ) {
        // the first 20 points of encumbrance combined is ignored
        encumb += std::max( 0, guy.encumb( bodypart_id( "leg_l" ) ) + guy.encumb(
                                bodypart_id( "leg_r" ) ) - 20 );
    }
    if( sp.has_flag( spell_flag::SOMATIC ) ) {
        // the first 20 points of encumbrance combined is ignored
        encumb += std::max( 0, guy.encumb( bodypart_id( "arm_l" ) ) + guy.encumb(
                                bodypart_id( "arm_r" ) ) - 20 );
    }
    return encumb > 0;
}

static bool energy_cost_encumbered( const spell &sp, const Character &guy )
{
    if( !sp.has_flag( spell_flag::NO_HANDS ) ) {
        return std::max( 0, guy.encumb( bodypart_id( "hand_l" ) ) + guy.encumb(
                             bodypart_id( "hand_r" ) ) - 10 ) >
               0;
    }
    return false;
}

// this prints various things about the spell out in a list
// including flags and things like "goes through walls"
static std::string enumerate_spell_data( const spell &sp )
{
    std::vector<std::string> spell_data;
    if( sp.has_flag( spell_flag::CONCENTRATE ) ) {
        spell_data.emplace_back( _( "requires concentration" ) );
    }
    if( sp.has_flag( spell_flag::VERBAL ) ) {
        spell_data.emplace_back( _( "verbal" ) );
    }
    if( sp.has_flag( spell_flag::SOMATIC ) ) {
        spell_data.emplace_back( _( "somatic" ) );
    }
    if( !sp.has_flag( spell_flag::NO_HANDS ) ) {
        spell_data.emplace_back( _( "impeded by gloves" ) );
    } else {
        spell_data.emplace_back( _( "does not require hands" ) );
    }
    if( !sp.has_flag( spell_flag::NO_LEGS ) ) {
        spell_data.emplace_back( _( "requires mobility" ) );
    }
    if( sp.effect() == "target_attack" && sp.range() > 1 ) {
        spell_data.emplace_back( _( "can be cast through walls" ) );
    }
    return enumerate_as_string( spell_data );
}

void spellcasting_callback::draw_spell_info( const spell &sp, const uilist *menu )
{
    const int h_offset = menu->w_width - menu->pad_right + 1;
    // includes spaces on either side for readability
    const int info_width = menu->pad_right - 4;
    const int win_height = menu->w_height;
    const int h_col1 = h_offset + 1;
    const int h_col2 = h_offset + ( info_width / 2 );
    const catacurses::window w_menu = menu->window;
    // various pieces of spell data mean different things depending on the effect of the spell
    const std::string fx = sp.effect();
    int line = 1;
    nc_color gray = c_light_gray;
    nc_color light_green = c_light_green;
    nc_color yellow = c_yellow;

    print_colored_text( w_menu, point( h_col1, line++ ), yellow, yellow,
                        sp.spell_class() == trait_NONE ? _( "Classless" ) : sp.spell_class()->name() );

    line += fold_and_print( w_menu, point( h_col1, line ), info_width, gray, sp.description() );

    line++;

    line += fold_and_print( w_menu, point( h_col1, line ), info_width, gray,
                            enumerate_spell_data( sp ) );
    if( line <= win_height / 3 ) {
        line++;
    }

    print_colored_text( w_menu, point( h_col1, line ), gray, gray,
                        string_format( "%s: %d %s", _( "Spell Level" ), sp.get_level(),
                                       sp.is_max_level() ? _( "(MAX)" ) : "" ) );
    print_colored_text( w_menu, point( h_col2, line++ ), gray, gray,
                        string_format( "%s: %d", _( "Max Level" ), sp.get_max_level() ) );

    print_colored_text( w_menu, point( h_col1, line ), gray, gray,
                        sp.colorized_fail_percent( get_player_character() ) );
    print_colored_text( w_menu, point( h_col2, line++ ), gray, gray,
                        string_format( "%s: %d", _( "Difficulty" ), sp.get_difficulty() ) );

    print_colored_text( w_menu, point( h_col1, line ), gray, gray,
                        string_format( "%s: %s", _( "Current Exp" ), colorize( to_string( sp.xp() ), light_green ) ) );
    print_colored_text( w_menu, point( h_col2, line++ ), gray, gray,
                        string_format( "%s: %s", _( "to Next Level" ), colorize( to_string( sp.exp_to_next_level() ),
                                       light_green ) ) );

    if( line <= win_height / 2 ) {
        line++;
    }

    const bool cost_encumb = energy_cost_encumbered( sp, get_player_character() );
    std::string cost_string = cost_encumb ? _( "Casting Cost (impeded)" ) : _( "Casting Cost" );
    std::string energy_cur = sp.energy_source() == magic_energy_type::hp ? "" :
                             string_format( _( " (%s current)" ), sp.energy_cur_string( get_player_character() ) );
    if( !sp.can_cast( get_player_character() ) ) {
        cost_string = colorize( _( "Not Enough Energy" ), c_red );
        energy_cur = "";
    }
    print_colored_text( w_menu, point( h_col1, line++ ), gray, gray,
                        string_format( "%s: %s %s%s", cost_string,
                                       sp.energy_cost_string( get_player_character() ), sp.energy_string(), energy_cur ) );
    const bool c_t_encumb = casting_time_encumbered( sp, get_player_character() );
    print_colored_text( w_menu, point( h_col1, line++ ), gray, gray, colorize(
                            string_format( "%s: %s", c_t_encumb ? _( "Casting Time (impeded)" ) : _( "Casting Time" ),
                                           moves_to_string( sp.casting_time( get_player_character() ) ) ),
                            c_t_encumb  ? c_red : c_light_gray ) );

    if( line <= win_height * 3 / 4 ) {
        line++;
    }

    std::string targets;
    if( sp.is_valid_target( spell_target::none ) ) {
        targets = _( "self" );
    } else {
        targets = sp.enumerate_targets();
    }
    print_colored_text( w_menu, point( h_col1, line++ ), gray, gray,
                        string_format( "%s: %s", _( "Valid Targets" ), targets ) );

    std::string target_ids;
    target_ids = sp.list_targeted_monster_names();
    if( !target_ids.empty() ) {
        fold_and_print( w_menu, point( h_col1, line++ ), info_width, gray,
                        _( "Only affects the monsters: %s" ), target_ids );
    }

    if( line <= win_height * 3 / 4 ) {
        line++;
    }

    const int damage = sp.damage();
    std::string damage_string;
    std::string range_string;
    std::string aoe_string;
    // if it's any type of attack spell, the stats are normal.
    if( fx == "target_attack" || fx == "projectile_attack" || fx == "cone_attack" ||
        fx == "line_attack" ) {
        if( damage > 0 ) {
            std::string dot_string;
            if( sp.damage_dot() != 0 ) {
                //~ amount of damage per second, abbreviated
                dot_string = string_format( _( ", %d/sec" ), sp.damage_dot() );
            }
            damage_string = string_format( "%s: %s %s%s", _( "Damage" ), sp.damage_string(),
                                           sp.damage_type_string(), dot_string );
            damage_string = colorize( damage_string, sp.damage_type_color() );
        } else if( damage < 0 ) {
            damage_string = string_format( "%s: %s", _( "Healing" ), colorize( sp.damage_string(),
                                           light_green ) );
        }
        if( sp.aoe() > 0 ) {
            std::string aoe_string_temp = _( "Spell Radius" );
            std::string degree_string;
            if( fx == "cone_attack" ) {
                aoe_string_temp = _( "Cone Arc" );
                degree_string = _( "degrees" );
            } else if( fx == "line_attack" ) {
                aoe_string_temp = _( "Line Width" );
            }
            aoe_string = string_format( "%s: %d %s", aoe_string_temp, sp.aoe(), degree_string );
        }
    } else if( fx == "teleport_random" ) {
        if( sp.aoe() > 0 ) {
            aoe_string = string_format( "%s: %d", _( "Variance" ), sp.aoe() );
        }
    } else if( fx == "spawn_item" ) {
        damage_string = string_format( "%s %d %s", _( "Spawn" ), sp.damage(),
                                       item::nname( itype_id( sp.effect_data() ), sp.damage() ) );
    } else if( fx == "summon" ) {
        std::string monster_name = "FIXME";
        if( sp.has_flag( spell_flag::SPAWN_GROUP ) ) {
            // TODO: Get a more user-friendly group name
            if( MonsterGroupManager::isValidMonsterGroup( mongroup_id( sp.effect_data() ) ) ) {
                monster_name = "from " + sp.effect_data();
            } else {
                debugmsg( "Unknown monster group: %s", sp.effect_data() );
            }
        } else {
            monster_name = monster( mtype_id( sp.effect_data() ) ).get_name( );
        }
        damage_string = string_format( "%s %d %s", _( "Summon" ), sp.damage(), _( monster_name ) );
        aoe_string = string_format( "%s: %d", _( "Spell Radius" ), sp.aoe() );
    } else if( fx == "targeted_polymorph" ) {
        std::string monster_name = sp.effect_data();
        if( sp.has_flag( spell_flag::POLYMORPH_GROUP ) ) {
            // TODO: Get a more user-friendly group name
            if( MonsterGroupManager::isValidMonsterGroup( mongroup_id( sp.effect_data() ) ) ) {
                monster_name = _( "random creature" );
            } else {
                debugmsg( "Unknown monster group: %s", sp.effect_data() );
            }
        } else if( monster_name.empty() ) {
            monster_name = _( "random creature" );
        } else {
            monster_name = mtype_id( sp.effect_data() )->nname();
        }
        damage_string = string_format( _( "Targets under: %dhp become a %s" ), sp.damage(),
                                       monster_name );
    } else if( fx == "ter_transform" ) {
        aoe_string = string_format( "%s: %s", _( "Spell Radius" ), sp.aoe_string() );
    }

    range_string = string_format( "%s: %s", _( "Range" ),
                                  sp.range() <= 0 ? _( "self" ) : to_string( sp.range() ) );

    // Range / AOE in two columns
    print_colored_text( w_menu, point( h_col1, line ), gray, gray, range_string );
    print_colored_text( w_menu, point( h_col2, line++ ), gray, gray, aoe_string );

    // One line for damage / healing / spawn / summon effect
    print_colored_text( w_menu, point( h_col1, line++ ), gray, gray, damage_string );

    // todo: damage over time here, when it gets implemeted

    print_colored_text( w_menu, point( h_col1, line++ ), gray, gray, sp.duration() <= 0 ? "" :
                        string_format( "%s: %s", _( "Duration" ), sp.duration_string() ) );

    // helper function for printing tool and item component requirement lists
    const auto print_vec_string = [&]( const std::vector<std::string> &vec ) {
        for( const std::string &line_str : vec ) {
            print_colored_text( w_menu, point( h_col1, line++ ), gray, gray, line_str );
        }
    };

    if( sp.has_components() ) {
        if( !sp.components().get_components().empty() ) {
            print_vec_string( sp.components().get_folded_components_list( info_width - 2, gray,
                              get_player_character().crafting_inventory(), return_true<item> ) );
        }
        if( !( sp.components().get_tools().empty() && sp.components().get_qualities().empty() ) ) {
            print_vec_string( sp.components().get_folded_tools_list( info_width - 2, gray,
                              get_player_character().crafting_inventory() ) );
        }
    }
}

bool known_magic::set_invlet( const spell_id &sp, int invlet, const std::set<int> &used_invlets )
{
    if( used_invlets.count( invlet ) > 0 ) {
        return false;
    }
    invlets[sp] = invlet;
    return true;
}

void known_magic::rem_invlet( const spell_id &sp )
{
    invlets.erase( sp );
}

int known_magic::get_invlet( const spell_id &sp, std::set<int> &used_invlets )
{
    auto found = invlets.find( sp );
    if( found != invlets.end() ) {
        return found->second;
    }
    for( const std::pair<const spell_id, int> &invlet_pair : invlets ) {
        used_invlets.emplace( invlet_pair.second );
    }
    for( int i = 'a'; i <= 'z'; i++ ) {
        if( used_invlets.count( i ) == 0 ) {
            used_invlets.emplace( i );
            return i;
        }
    }
    for( int i = 'A'; i <= 'Z'; i++ ) {
        if( used_invlets.count( i ) == 0 ) {
            used_invlets.emplace( i );
            return i;
        }
    }
    for( int i = '!'; i <= '-'; i++ ) {
        if( used_invlets.count( i ) == 0 ) {
            used_invlets.emplace( i );
            return i;
        }
    }
    return 0;
}

int known_magic::select_spell( Character &guy )
{
    // max width of spell names
    const int max_spell_name_length = get_spellname_max_width();
    std::vector<spell *> known_spells = get_spells();

    uilist spell_menu;
    spell_menu.w_height_setup = [&]() -> int {
        return clamp( static_cast<int>( known_spells.size() ), 24, TERMY * 9 / 10 );
    };
    const auto calc_width = []() -> int {
        return std::max( 80, TERMX * 3 / 8 );
    };
    spell_menu.w_width_setup = calc_width;
    spell_menu.pad_right_setup = [&]() -> int {
        return calc_width() - max_spell_name_length - 5;
    };
    spell_menu.title = _( "Choose a Spell" );
    spell_menu.hilight_disabled = true;
    spellcasting_callback cb( known_spells, casting_ignore );
    spell_menu.callback = &cb;

    std::set<int> used_invlets{ cb.reserved_invlets };

    for( size_t i = 0; i < known_spells.size(); i++ ) {
        spell_menu.addentry( static_cast<int>( i ), known_spells[i]->can_cast( guy ),
                             get_invlet( known_spells[i]->id(), used_invlets ), known_spells[i]->name() );
    }

    spell_menu.query();

    casting_ignore = static_cast<spellcasting_callback *>( spell_menu.callback )->casting_ignore;

    return spell_menu.ret;
}

void known_magic::on_mutation_gain( const trait_id &mid, Character &guy )
{
    for( const std::pair<const spell_id, int> &sp : mid->spells_learned ) {
        learn_spell( sp.first, guy, true );
        spell &temp_sp = get_spell( sp.first );
        for( int level = 0; level < sp.second; level++ ) {
            temp_sp.gain_level();
        }
    }
}

void known_magic::on_mutation_loss( const trait_id &mid )
{
    std::vector<spell_id> spells_to_forget;
    for( const spell *sp : get_spells() ) {
        if( sp->is_spell_class( mid ) ) {
            spells_to_forget.emplace_back( sp->id() );
        }
    }
    for( const spell_id &sp_id : spells_to_forget ) {
        forget_spell( sp_id );
    }
}

void spellbook_callback::add_spell( const spell_id &sp )
{
    spells.emplace_back( sp.obj() );
}

static std::string color_number( const int num )
{
    if( num > 0 ) {
        return colorize( to_string( num ), c_light_green );
    } else if( num < 0 ) {
        return colorize( to_string( num ), c_light_red );
    } else {
        return colorize( to_string( num ), c_white );
    }
}

static std::string color_number( const float num )
{
    if( num > 100 ) {
        return colorize( string_format( "+%.0f", num ), c_light_green );
    } else if( num < -100 ) {
        return colorize( string_format( "%.0f", num ), c_light_red );
    } else if( num > 0 ) {
        return colorize( string_format( "+%.2f", num ), c_light_green );
    } else if( num < 0 ) {
        return colorize( string_format( "%.2f", num ), c_light_red );
    } else {
        return colorize( "0", c_white );
    }
}

static void draw_spellbook_info( const spell_type &sp, uilist *menu )
{
    const int width = menu->pad_left - 4;
    const int start_x = 2;
    int line = 1;
    const catacurses::window w = menu->window;
    nc_color gray = c_light_gray;
    nc_color yellow = c_yellow;
    const spell fake_spell( sp.id );

    const std::string spell_name = colorize( sp.name, c_light_green );
    const std::string spell_class = sp.spell_class == trait_NONE ? _( "Classless" ) :
                                    sp.spell_class->name();
    print_colored_text( w, point( start_x, line ), gray, gray, spell_name );
    print_colored_text( w, point( menu->pad_left - utf8_width( spell_class ) - 1, line++ ), yellow,
                        yellow, spell_class );
    line++;
    line += fold_and_print( w, point( start_x, line ), width, gray, "%s", sp.description );
    line++;

    mvwprintz( w, point( start_x, line ), c_light_gray, string_format( "%s: %d", _( "Difficulty" ),
               sp.difficulty ) );
    mvwprintz( w, point( start_x + width / 2, line++ ), c_light_gray, string_format( "%s: %d",
               _( "Max Level" ),
               sp.max_level ) );

    const std::string fx = sp.effect_name;
    std::string damage_string;
    std::string aoe_string;
    bool has_damage_type = false;
    if( fx == "target_attack" || fx == "projectile_attack" || fx == "cone_attack" ||
        fx == "line_attack" ) {
        damage_string = _( "Damage" );
        aoe_string = _( "AoE" );
        has_damage_type = sp.min_damage > 0 && sp.max_damage > 0;
    } else if( fx == "spawn_item" || fx == "summon_monster" ) {
        damage_string = _( "Spawned" );
    } else if( fx == "targeted_polymorph" ) {
        damage_string = _( "Threshold" );
    } else if( fx == "recover_energy" ) {
        damage_string = _( "Recover" );
    } else if( fx == "teleport_random" ) {
        aoe_string = _( "Variance" );
    } else if( fx == "area_pull" || fx == "area_push" ||  fx == "ter_transform" ) {
        aoe_string = _( "AoE" );
    }

    if( has_damage_type ) {
        print_colored_text( w, point( start_x, line++ ), gray, gray, string_format( "%s: %s",
                            _( "Damage Type" ),
                            colorize( fake_spell.damage_type_string(), fake_spell.damage_type_color() ) ) );
    }
    line++;

    print_colored_text( w, point( start_x, line++ ), gray, gray,
                        string_format( "%s %s %s %s",
                                       //~ translation should not exceed 10 console cells
                                       left_justify( _( "Stat Gain" ), 10 ),
                                       //~ translation should not exceed 7 console cells
                                       left_justify( _( "lvl 0" ), 7 ),
                                       //~ translation should not exceed 7 console cells
                                       left_justify( _( "per lvl" ), 7 ),
                                       //~ translation should not exceed 7 console cells
                                       left_justify( _( "max lvl" ), 7 ) ) );
    std::vector<std::tuple<std::string, int, float, int>> rows;

    if( sp.max_damage != 0 && sp.min_damage != 0 && !damage_string.empty() ) {
        rows.emplace_back( damage_string, sp.min_damage, sp.damage_increment, sp.max_damage );
    }

    if( sp.max_range != 0 && sp.min_range != 0 ) {
        rows.emplace_back( _( "Range" ), sp.min_range, sp.range_increment, sp.max_range );
    }

    if( sp.min_aoe != 0 && sp.max_aoe != 0 && !aoe_string.empty() ) {
        rows.emplace_back( aoe_string, sp.min_aoe, sp.aoe_increment, sp.max_aoe );
    }

    if( sp.min_duration != 0 && sp.max_duration != 0 ) {
        rows.emplace_back( _( "Duration" ), sp.min_duration, static_cast<float>( sp.duration_increment ),
                           sp.max_duration );
    }

    rows.emplace_back( _( "Cast Cost" ), sp.base_energy_cost, sp.energy_increment,
                       sp.final_energy_cost );
    rows.emplace_back( _( "Cast Time" ), sp.base_casting_time, sp.casting_time_increment,
                       sp.final_casting_time );

    for( std::tuple<std::string, int, float, int> &row : rows ) {
        mvwprintz( w, point( start_x, line ), c_light_gray, std::get<0>( row ) );
        print_colored_text( w, point( start_x + 11, line ), gray, gray,
                            color_number( std::get<1>( row ) ) );
        print_colored_text( w, point( start_x + 19, line ), gray, gray,
                            color_number( std::get<2>( row ) ) );
        print_colored_text( w, point( start_x + 27, line ), gray, gray,
                            color_number( std::get<3>( row ) ) );
        line++;
    }
}

void spellbook_callback::refresh( uilist *menu )
{
    mvwputch( menu->window, point( menu->pad_left, 0 ), c_magenta, LINE_OXXX );
    mvwputch( menu->window, point( menu->pad_left, menu->w_height - 1 ), c_magenta, LINE_XXOX );
    for( int i = 1; i < menu->w_height - 1; i++ ) {
        mvwputch( menu->window, point( menu->pad_left, i ), c_magenta, LINE_XOXO );
    }
    if( menu->selected >= 0 && static_cast<size_t>( menu->selected ) < spells.size() ) {
        draw_spellbook_info( spells[menu->selected], menu );
    }
    wnoutrefresh( menu->window );
}

void fake_spell::load( const JsonObject &jo )
{
    std::string temp_id;
    mandatory( jo, false, "id", temp_id );
    id = spell_id( temp_id );
    optional( jo, false, "hit_self", self, false );

    optional( jo, false, "once_in", trigger_once_in, 1 );
    optional( jo, false, "message", trigger_message );
    optional( jo, false, "npc_message", npc_trigger_message );
    int max_level_int;
    optional( jo, false, "max_level", max_level_int, -1 );
    if( max_level_int == -1 ) {
        max_level = cata::nullopt;
    } else {
        max_level = max_level_int;
    }
    optional( jo, false, "min_level", level, 0 );
    if( jo.has_string( "level" ) ) {
        debugmsg( "level member for fake_spell was renamed to min_level.  id: %s", temp_id );
    }
}

void fake_spell::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "id", id );
    json.member( "hit_self", self );
    json.member( "once_in", trigger_once_in );

    if( !max_level ) {
        json.member( "max_level", -1 );
    } else {
        json.member( "max_level", *max_level );
    }
    json.member( "min_level", level );
    json.end_object();
}

void fake_spell::deserialize( JsonIn &jsin )
{
    JsonObject data = jsin.get_object();
    load( data );
}

spell fake_spell::get_spell( int min_level_override ) const
{
    spell sp( id );
    // the max level this spell will be. can be optionally limited
    int spell_limiter = max_level ? std::min( *max_level, sp.get_max_level() ) : sp.get_max_level();
    // level is the minimum level the fake_spell will output
    min_level_override = std::max( min_level_override, level );
    if( min_level_override > spell_limiter ) {
        // this override is for min level, and does not override max level
        min_level_override = spell_limiter;
    }
    // the "level" of the fake spell is the goal, but needs to be clamped to min and max
    int level_of_spell = clamp( level, min_level_override,  std::min( sp.get_max_level(),
                                spell_limiter ) );
    if( level > spell_limiter ) {
        debugmsg( "ERROR: fake spell %s has higher min_level than max_level", id.c_str() );
        return sp;
    }
    while( sp.get_level() < level_of_spell ) {
        sp.gain_level();
    }
    return sp;
}

void spell_events::notify( const cata::event &e )
{
    switch( e.type() ) {
        case event_type::player_levels_spell: {
            spell_id sid = e.get<spell_id>( "spell" );
            int slvl = e.get<int>( "new_level" );
            spell_type spell_cast = spell_factory.obj( sid );
            for( std::map<std::string, int>::iterator it = spell_cast.learn_spells.begin();
                 it != spell_cast.learn_spells.end(); ++it ) {
                int learn_at_level = it->second;
                if( learn_at_level == slvl ) {
                    std::string learn_spell_id = it->first;
                    get_player_character().magic->learn_spell( learn_spell_id, get_player_character() );
                    spell_type spell_learned = spell_factory.obj( spell_id( learn_spell_id ) );
                    add_msg(
                        _( "Your experience and knowledge in creating and manipulating magical energies to cast %s have opened your eyes to new possibilities, you can now cast %s." ),
                        spell_cast.name,
                        spell_learned.name );
                }
            }
            break;
        }
        default:
            break;

    }
}
