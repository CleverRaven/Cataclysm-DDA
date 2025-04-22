#include "weakpoint.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <utility>

#include "assign.h"
#include "calendar.h"
#include "character.h"
#include "condition.h"
#include "creature.h"
#include "damage.h"
#include "debug.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "effect_source.h"
#include "enums.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "item.h"
#include "magic_enchantment.h"
#include "make_static.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "pimpl.h"
#include "proficiency.h"
#include "rng.h"
#include "talker.h"
#include "translations.h"

static const limb_score_id limb_score_reaction( "reaction" );
static const limb_score_id limb_score_vision( "vision" );

static const skill_id skill_gun( "gun" );
static const skill_id skill_melee( "melee" );
static const skill_id skill_throw( "throw" );
static const skill_id skill_unarmed( "unarmed" );

namespace
{

generic_factory<weakpoints> weakpoints_factory( "weakpoint sets" );

} // namespace

/** @relates string_id */
template<>
const weakpoints &string_id<weakpoints>::obj() const
{
    return weakpoints_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<weakpoints>::is_valid() const
{
    return weakpoints_factory.is_valid( *this );
}

void weakpoints::load_weakpoint_sets( const JsonObject &jo, const std::string &src )
{
    weakpoints_factory.load( jo, src );
}

void weakpoints::reset()
{
    weakpoints_factory.reset();
}

const std::vector<weakpoints> &weakpoints::get_all()
{
    return weakpoints_factory.get_all();
}

float monster::weakpoint_skill() const
{
    return type->melee_skill;
}

float Character::generic_weakpoint_skill( skill_id skill_1, skill_id skill_2,
        limb_score_id limb_score_1, limb_score_id limb_score_2 ) const
{
    float skill = ( get_skill_level( skill_1 ) + get_skill_level( skill_2 ) ) / 2.0;
    float stat = ( get_dex() - 8 ) / 8.0 + ( get_per() - 8 ) / 8.0;
    float mul = ( get_limb_score( limb_score_1 ) + get_limb_score( limb_score_2 ) ) / 2.0;
    return ( skill + stat ) * mul;
}

float Character::melee_weakpoint_skill( const item &weapon ) const
{
    return generic_weakpoint_skill( weapon.is_null() ? skill_unarmed : weapon.melee_skill(),
                                    skill_melee, limb_score_vision, limb_score_reaction );
}

float Character::ranged_weakpoint_skill( const item &weapon ) const
{
    return generic_weakpoint_skill( weapon.gun_skill(), skill_gun, limb_score_vision,
                                    limb_score_vision );
}

float Character::throw_weakpoint_skill() const
{
    return generic_weakpoint_skill( skill_throw, skill_throw, limb_score_vision, limb_score_vision );
}

float weakpoint_family::modifier( const Character &attacker ) const
{
    return attacker.has_proficiency( proficiency )
           ? bonus.value_or( proficiency.obj().default_weakpoint_bonus() )
           : penalty.value_or( proficiency.obj().default_weakpoint_penalty() );
}

void weakpoint_family::load( const JsonValue &jsin )
{
    if( jsin.test_string() ) {
        id = jsin.get_string();
        proficiency = proficiency_id( id );
    } else {
        JsonObject jo = jsin.get_object();
        assign( jo, "id", id );
        assign( jo, "proficiency", proficiency );
        assign( jo, "bonus", bonus );
        assign( jo, "penalty", penalty );
        if( !jo.has_string( "id" ) ) {
            id = static_cast<std::string>( proficiency );
        }
    }
}

bool weakpoint_families::practice( Character &learner, const time_duration &amount ) const
{
    bool learned = false;
    for( const weakpoint_family &family : families ) {
        float before = learner.get_proficiency_practice( family.proficiency );
        learner.practice_proficiency( family.proficiency, amount );
        float after = learner.get_proficiency_practice( family.proficiency );
        if( before < after ) {
            learned = true;
        }
    }
    return learned;
}

bool weakpoint_families::practice_hit( Character &learner ) const
{
    return practice( learner, time_duration::from_seconds( 2 ) );
}

bool weakpoint_families::practice_kill( Character &learner ) const
{
    return practice( learner, time_duration::from_minutes( 1 ) );
}

bool weakpoint_families::practice_dissect( Character &learner ) const
{
    // Proficiency experience is capped at 1000 seconds (~16 minutes), so we split it into two
    // instances. This should be refactored when butchering becomes an `activity_actor`.
    bool p1 = practice( learner, time_duration::from_minutes( 15 ) );
    bool p2 = practice( learner, time_duration::from_minutes( 15 ) );
    bool learned = p1 || p2;
    if( learned ) {
        learner.add_msg_if_player(
            m_good, _( "You carefully record the creature's vulnerabilities." ) );
    }
    return learned;
}

float weakpoint_families::modifier( const Character &attacker ) const
{
    float total = 0.0f;
    for( const weakpoint_family &family : families ) {
        total += family.modifier( attacker );
    }
    return total;
}

void weakpoint_families::clear()
{
    families.clear();
}

void weakpoint_families::load( const JsonArray &ja )
{
    for( const JsonValue jsin : ja ) {
        weakpoint_family tmp;
        tmp.load( jsin );

        auto it = std::find_if( families.begin(), families.end(),
        [&]( const weakpoint_family & wf ) {
            return wf.id == tmp.id;
        } );
        if( it != families.end() ) {
            families.erase( it );
        }

        families.push_back( std::move( tmp ) );
    }
}

void weakpoint_families::remove( const JsonArray &ja )
{
    for( const JsonValue jsin : ja ) {
        weakpoint_family tmp;
        tmp.load( jsin );

        auto it = std::find_if( families.begin(), families.end(),
        [&]( const weakpoint_family & wf ) {
            return wf.id == tmp.id;
        } );
        if( it != families.end() ) {
            families.erase( it );
        }
    }
}

weakpoint_difficulty::weakpoint_difficulty( float default_value )
{
    difficulty.fill( default_value );
}

float weakpoint_difficulty::of( const weakpoint_attack &attack ) const
{
    return difficulty[static_cast<int>( attack.type )];
}

void weakpoint_difficulty::load( const JsonObject &jo )
{
    using attack_type = weakpoint_attack::attack_type;
    float default_value = difficulty[static_cast<int>( attack_type::NONE )];
    float all = jo.get_float( "all", default_value );
    // Determine default values
    float bash;
    float cut;
    float stab;
    float ranged = all;
    // Support either "melee" shorthand or "broad"/"point" shorthand.
    if( jo.has_float( "melee" ) ) {
        float melee = jo.get_float( "melee", all );
        bash = melee;
        cut = melee;
        stab = melee;
    } else {
        float broad = jo.get_float( "broad", all );
        float point = jo.get_float( "point", all );
        bash = broad;
        cut = broad;
        stab = point;
        ranged = point;
    }
    // Load the values
    difficulty[static_cast<int>( attack_type::NONE )] = all;
    difficulty[static_cast<int>( attack_type::MELEE_BASH )] = jo.get_float( "bash", bash );
    difficulty[static_cast<int>( attack_type::MELEE_CUT )] = jo.get_float( "cut", cut );
    difficulty[static_cast<int>( attack_type::MELEE_STAB )] = jo.get_float( "stab", stab );
    difficulty[static_cast<int>( attack_type::PROJECTILE )] = jo.get_float( "ranged", ranged );
}

weakpoint_effect::weakpoint_effect()  :
    chance( 100.0f ),
    permanent( false ),
    duration( 1, 1 ),
    intensity( 0, 0 ),
    damage_required( 0.0f, 100.0f ) {}

std::string weakpoint_effect::get_message() const
{
    return message.translated();
}

void weakpoint_effect::apply_to( Creature &target, int total_damage,
                                 const weakpoint_attack &attack ) const
{
    // Check if damage is within required bounds
    float percent_hp = 100.0f * static_cast<float>( total_damage ) / target.get_hp_max();
    percent_hp = std::min( 100.0f, percent_hp );
    if( percent_hp < damage_required.first || damage_required.second < percent_hp ) {
        return;
    }
    // Roll for chance.
    if( !( rng_float( 0.0f, 100.f ) < chance ) ) {
        return;
    }
    if( effect && !effect.is_empty() ) {
        target.add_effect( effect_source( attack.source ), effect,
                           time_duration::from_turns( rng( duration.first, duration.second ) ),
                           permanent, rng( intensity.first, intensity.second ) );
    }
    for( const effect_on_condition_id &eoc : effect_on_conditions ) {
        dialogue d( attack.source == nullptr ? nullptr : get_talker_for( *attack.source ),
                    get_talker_for( target ) );
        eoc->activate( d );
    }

    if( !get_message().empty() && attack.source != nullptr && attack.source->is_avatar() ) {
        add_msg_if_player_sees( target, m_good, get_message(), target.get_name() );
    }
}

void weakpoint_effect::load( const JsonObject &jo )
{
    if( jo.has_string( "effect" ) ) {
        assign( jo, "effect", effect );
    }
    if( jo.has_array( "effect_on_conditions" ) ) {
        assign( jo, "effect_on_conditions", effect_on_conditions );
    }
    if( jo.has_float( "chance" ) ) {
        assign( jo, "chance", chance, false, 0.0f, 100.0f );
    }
    if( jo.has_bool( "permanent" ) ) {
        assign( jo, "permanent", permanent );
    }
    if( jo.has_string( "message" ) ) {
        assign( jo, "message", message );
    }

    // Support shorthand for a single value.
    if( jo.has_int( "duration" ) ) {
        int i = jo.get_int( "duration", 0 );
        duration = {i, i};
    } else if( jo.has_array( "duration" ) ) {
        assign( jo, "duration", duration );
    }
    if( jo.has_int( "intensity" ) ) {
        int i = jo.get_int( "intensity", 0 );
        intensity = {i, i};
    } else if( jo.has_array( "intensity" ) ) {
        assign( jo, "intensity", intensity );
    }
    if( jo.has_float( "damage_required" ) ) {
        float f = jo.get_float( "damage_required", 0.0f );
        damage_required = {f, f};
    } else if( jo.has_array( "damage_required" ) ) {
        assign( jo, "damage_required", damage_required );
    }
}

weakpoint_attack::weakpoint_attack()  :
    source( nullptr ),
    target( nullptr ),
    weapon( &null_item_reference() ),
    type( attack_type::NONE ),
    is_thrown( false ),
    is_crit( false ),
    wp_skill( 0.0f ) {}

weakpoint_attack::attack_type
weakpoint_attack::type_of_melee_attack( const damage_instance &damage )
{
    damage_type_id primary = damage_type_id::NULL_ID();
    int primary_amount = 0;
    for( const damage_unit &du : damage.damage_units ) {
        if( du.amount > primary_amount ) {
            primary = du.type;
            primary_amount = du.amount;
        }
    }
    // FIXME: Hardcoded damage types
    if( primary == STATIC( damage_type_id( "bash" ) ) ) {
        return attack_type::MELEE_BASH;
    } else if( primary == STATIC( damage_type_id( "cut" ) ) ) {
        return attack_type::MELEE_CUT;
    } else if( primary == STATIC( damage_type_id( "stab" ) ) ) {
        return attack_type::MELEE_STAB;
    }
    return attack_type::NONE;
}

void weakpoint_attack::compute_wp_skill()
{
    // Check if there's no source.
    if( source == nullptr ) {
        wp_skill = 0.0f;
        return;
    }
    // Compute the base attacker skill.
    float attacker_skill = 0.0f;
    const monster *mon_att = source->as_monster();
    const Character *chr_att = source->as_character();
    if( mon_att != nullptr ) {
        attacker_skill = mon_att->weakpoint_skill();
    } else if( chr_att != nullptr ) {
        switch( type ) {
            case attack_type::MELEE_BASH:
            case attack_type::MELEE_CUT:
            case attack_type::MELEE_STAB:
                attacker_skill = chr_att->melee_weakpoint_skill( *weapon );
                break;
            case attack_type::PROJECTILE:
                attacker_skill = is_thrown
                                 ? chr_att->throw_weakpoint_skill()
                                 : chr_att->ranged_weakpoint_skill( *weapon );
                break;
            default:
                attacker_skill = 0.0f;
                break;
        }
    }
    // Compute the proficiency skill.
    float proficiency_skill = 0.0f;
    const monster *mon_tar = target->as_monster();
    if( chr_att != nullptr && mon_tar != nullptr ) {
        proficiency_skill = mon_tar->type->families.modifier( *chr_att );
    }
    // Combine attacker skill and proficiency boni.
    wp_skill = attacker_skill + proficiency_skill;
}

weakpoint::weakpoint() : coverage_mult( 1.0f ), difficulty( -100.0f )
{

}

void weakpoint::load( const JsonObject &jo )
{
    assign( jo, "id", id );
    assign( jo, "name", name );
    assign( jo, "coverage", coverage, false, 0.0f, 100.0f );
    if( jo.has_bool( "is_good" ) ) {
        assign( jo, "is_good", is_good );
    }
    if( is_good && jo.has_bool( "is_head" ) ) {
        assign( jo, "is_head", is_head );
    }
    if( jo.has_object( "armor_mult" ) ) {
        armor_mult = load_damage_map( jo.get_object( "armor_mult" ) );
    }
    if( jo.has_object( "armor_penalty" ) ) {
        armor_penalty = load_damage_map( jo.get_object( "armor_penalty" ) );
    }
    if( jo.has_object( "damage_mult" ) ) {
        damage_mult = load_damage_map( jo.get_object( "damage_mult" ) );
    }
    if( jo.has_object( "crit_mult" ) ) {
        crit_mult = load_damage_map( jo.get_object( "crit_mult" ) );
    } else {
        // Default to damage multiplier, if crit multipler is not specified.
        crit_mult = damage_mult;
    }
    if( jo.has_member( "condition" ) ) {
        read_condition( jo, "condition", condition, false );
        has_condition = true;
    }
    if( jo.has_array( "effects" ) ) {
        for( const JsonObject effect_jo : jo.get_array( "effects" ) ) {
            weakpoint_effect effect;
            effect.load( effect_jo );
            effects.push_back( std::move( effect ) );
        }
    }
    if( jo.has_object( "coverage_mult" ) ) {
        coverage_mult.load( jo.get_object( "coverage_mult" ) );
    }
    if( jo.has_object( "difficulty" ) ) {
        difficulty.load( jo.get_object( "difficulty" ) );
    }

    // Set the ID to the name, if not provided.
    if( !jo.has_string( "id" ) ) {
        assign( jo, "name", id );
    }
}

void weakpoint::check() const
{
    for( const std::pair<const damage_type_id, float> &dt : armor_mult ) {
        if( !dt.first.is_valid() ) {
            debugmsg( "Invalid armor_mult type \"%s\" for weakpoint %s", dt.first.c_str(), id );
        }
    }
    for( const std::pair<const damage_type_id, float> &dt : armor_penalty ) {
        if( !dt.first.is_valid() ) {
            debugmsg( "Invalid armor_penalty type \"%s\" for weakpoint %s", dt.first.c_str(), id );
        }
    }
    for( const std::pair<const damage_type_id, float> &dt : damage_mult ) {
        if( !dt.first.is_valid() ) {
            debugmsg( "Invalid damage_mult type \"%s\" for weakpoint %s", dt.first.c_str(), id );
        }
    }
    for( const std::pair<const damage_type_id, float> &dt : crit_mult ) {
        if( !dt.first.is_valid() ) {
            debugmsg( "Invalid crit_mult type \"%s\" for weakpoint %s", dt.first.c_str(), id );
        }
    }
}

std::string weakpoint::get_name() const
{
    return name.translated();
}

void weakpoint::apply_to( resistances &resistances ) const
{
    for( const damage_type &dt : damage_type::get_all() ) {
        if( resistances.resist_vals.count( dt.id ) <= 0 ) {
            resistances.resist_vals[dt.id] = 0.0f;
        }
        if( armor_mult.count( dt.id ) > 0 ) {
            resistances.resist_vals[dt.id] *= armor_mult.at( dt.id );
        }
        if( armor_penalty.count( dt.id ) > 0 ) {
            resistances.resist_vals[dt.id] -= armor_penalty.at( dt.id );
        }
    }
}

void weakpoint::apply_to( damage_instance &damage, bool is_crit ) const
{
    for( damage_unit &elem : damage.damage_units ) {
        if( is_crit ) {
            if( crit_mult.count( elem.type ) > 0 ) {
                elem.damage_multiplier *= crit_mult.at( elem.type );
                add_msg_debug( debugmode::DF_MONSTER, "%s crit_mult %f",
                               elem.type.str(), crit_mult.at( elem.type ) );
            }
        } else if( damage_mult.count( elem.type ) > 0 ) {
            elem.damage_multiplier *= damage_mult.at( elem.type );
            add_msg_debug( debugmode::DF_MONSTER, "%s damage_mult %f",
                           elem.type.str(), damage_mult.at( elem.type ) );
        }
    }
}

void weakpoint::apply_effects( Creature &target, int total_damage,
                               const weakpoint_attack &attack ) const
{
    for( const weakpoint_effect &effect : effects ) {
        effect.apply_to( target, total_damage, attack );
    }
}

float weakpoint::hit_chance( const weakpoint_attack &attack ) const
{
    // Evaluate condition
    if( has_condition ) {
        dialogue d( attack.source == nullptr ? nullptr : get_talker_for( *attack.source ),
                    get_talker_for( *attack.target ) );
        if( !condition( d ) ) {
            add_msg_debug( debugmode::DF_MONSTER, "Attack conditionals failed" );
            return 0.0f;
        }
    }

    // Retrieve multipliers.
    float constant_mult = coverage_mult.of( attack );
    float diff;
    float difficulty_mult;
    float final_coverage;
    if( is_good ) {
        // Probability of a sample from a normal distribution centered on `skill` with `SD = 2`
        // exceeding the difficulty.
        diff = attack.wp_skill - difficulty.of( attack );
        difficulty_mult = 0.5f * ( 1.0f + erf( diff / ( 2.0f * sqrt( 2.0f ) ) ) );
        if( attack.source && attack.source->as_character() ) {
            final_coverage = attack.source->as_character()->enchantment_cache->modify_value(
                                 enchant_vals::mod::WEAKPOINT_ACCURACY, coverage );
        } else {
            final_coverage = coverage;
        }
    } else {
        // Use erfc if the wp does not benefit the attacker.
        diff = attack.wp_skill - std::max( difficulty.of( attack ) + 10.0f, 10.0f );
        difficulty_mult = std::max( 0.5f * erfc( diff / ( 2.0f * sqrt( 2.0f ) ) ), 0.1f );
        final_coverage = coverage;
    }

    return constant_mult * difficulty_mult * final_coverage;
}

// Reweighs the probability distribution of hitting a weakpoint.
//
// The value returned is the probability of sampling at least one value less than `base`
// from `rolls` uniform distributions over [0, 1].
//
// For hard to hit weak points, this multiplies their hit chance by `rolls`. A 1% base
// combined with 5 rolls has a 4.9% weight. However, even at high `rolls`, the attacker
// still has a chance of hitting armor. A 75% chance of not hitting a weak point becomes
// a 23.7% chance with 5 rolls.
static float reweigh( float base, float rolls )
{
    return 1.0f - pow( 1.0f - base, rolls );
}

const weakpoint *weakpoints::select_weakpoint( const weakpoint_attack &attack ) const
{
    add_msg_debug( debugmode::DF_MONSTER,
                   "Weakpoint Selection: Source: %s, Weapon %s, Skill %.3f, Accuracy %.3f",
                   attack.source == nullptr ? "nullptr" : attack.source->get_name(),
                   attack.weapon == nullptr ? "nullptr" : attack.weapon->type_name(),
                   attack.wp_skill, attack.accuracy );
    float rolls = std::max( 1.0f, 1.0f + attack.wp_skill / 2.5f );
    // The base probability of hitting a more preferable weak point.
    float base = 0.0f;
    // The reweighed probability of hitting a more preferable weak point.
    float reweighed = 0.0f;
    float idx = rng_float( 0.0f, 100.0f );
    for( const weakpoint &weakpoint : weakpoint_list ) {
        float raw_chance = weakpoint.hit_chance( attack );
        if( raw_chance == 0.0f ) {
            add_msg_debug( debugmode::DF_MONSTER,
                           "Weakpoint Selection: weakpoint %s, conditions not match",
                           weakpoint.id );
            continue;
        }
        float new_base = base + raw_chance;
        float new_reweighed = 100.0f * reweigh( new_base / 100.0f, rolls );
        float hit_chance = new_reweighed - reweighed;
        add_msg_debug( debugmode::DF_MONSTER,
                       "Weakpoint Selection: weakpoint %s, raw_chance %.4f, hit_chance %.4f",
                       weakpoint.id, raw_chance, hit_chance );
        if( idx < hit_chance ) {
            return &weakpoint;
        }
        base = new_base;
        reweighed = new_reweighed;
        idx -= hit_chance;
    }
    return &default_weakpoint;
}

void weakpoints::clear()
{
    weakpoint_list.clear();
}

void weakpoints::load( const JsonArray &ja )
{
    for( const JsonObject jo : ja ) {
        weakpoint tmp;
        tmp.load( jo );

        if( tmp.id.empty() ) {
            default_weakpoint = tmp;
            continue;
        }

        // Ensure that every weakpoint has a unique ID
        auto it = std::find_if( weakpoint_list.begin(), weakpoint_list.end(),
        [&]( const weakpoint & wp ) {
            return wp.id == tmp.id;
        } );
        if( it != weakpoint_list.end() ) {
            weakpoint_list.erase( it );
        }

        weakpoint_list.push_back( std::move( tmp ) );
    }
    // Prioritizes weakpoints based on their coverage.
    std::sort( weakpoint_list.begin(), weakpoint_list.end(),
    []( const weakpoint & a, const weakpoint & b ) {
        return a.coverage < b.coverage;
    } );
}

void weakpoints::check() const
{
    for( const weakpoint &w : weakpoint_list ) {
        w.check();
    }
}

void weakpoints::finalize()
{
    for( weakpoint &w : weakpoint_list ) {
        finalize_damage_map( w.armor_mult, false, 1.0f );
        finalize_damage_map( w.armor_penalty, false, 0.0f );
        finalize_damage_map( w.damage_mult, false, 1.0f );
        finalize_damage_map( w.crit_mult, false, 1.0f );
    }
}

void weakpoints::remove( const JsonArray &ja )
{
    for( const JsonObject jo : ja ) {
        weakpoint tmp;
        tmp.load( jo );

        if( tmp.id.empty() ) {
            default_weakpoint = weakpoint();
            continue;
        }

        auto it = std::find_if( weakpoint_list.begin(), weakpoint_list.end(),
        [&]( const weakpoint & wp ) {
            return wp.id == tmp.id;
        } );
        if( it != weakpoint_list.end() ) {
            weakpoint_list.erase( it );
        }
    }
}

void weakpoints::load( const JsonObject &jo, std::string_view )
{
    load( jo.get_array( "weakpoints" ) );
}

void weakpoints::finalize_all()
{
    weakpoints_factory.finalize();
}

void weakpoints::add_from_set( const weakpoints_id &set_id, bool replace_id )
{
    if( !set_id.is_valid() ) {
        debugmsg( "invalid weakpoint_set id \"%s\"", set_id.c_str() );
        return;
    }
    add_from_set( set_id.obj(), replace_id );
}

void weakpoints::add_from_set( const weakpoints &set, bool replace_id )
{
    for( const weakpoint &wp : set.weakpoint_list ) {
        auto iter = std::find_if( weakpoint_list.begin(),
        weakpoint_list.end(), [&wp]( const weakpoint & w ) {
            return w.id == wp.id && !w.id.empty();
        } );
        if( replace_id && iter != weakpoint_list.end() ) {
            weakpoint_list[iter - weakpoint_list.begin()] = wp;
        } else {
            weakpoint_list.emplace_back( wp );
        }
    }
}

void weakpoints::del_from_set( const weakpoints_id &set_id )
{
    if( !set_id.is_valid() ) {
        debugmsg( "invalid weakpoint_set id \"%s\"", set_id.c_str() );
        return;
    }
    del_from_set( set_id.obj() );
}

void weakpoints::del_from_set( const weakpoints &set )
{
    for( const weakpoint &wp : set.weakpoint_list ) {
        auto iter = std::find_if( weakpoint_list.begin(),
        weakpoint_list.end(), [&wp]( const weakpoint & w ) {
            return w.id == wp.id && !w.id.empty();
        } );
        if( iter != weakpoint_list.end() ) {
            weakpoint_list.erase( iter );
        }
    }
}
