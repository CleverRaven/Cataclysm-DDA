#include "skill.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>

#include "cata_utility.h"
#include "debug.h"
#include "item.h"
#include "json.h"
#include "options.h"
#include "recipe.h"
#include "rng.h"
#include "string_id.h"
#include "translations.h"

// TODO: a map, for Barry's sake make this a map.
std::vector<Skill> Skill::skills;
std::map<skill_id, Skill> Skill::contextual_skills;

std::vector<SkillDisplayType> SkillDisplayType::skillTypes;

static const Skill invalid_skill;
static const SkillDisplayType invalid_skill_type;

/** @relates string_id */
template<>
const Skill &string_id<Skill>::obj() const
{
    for( const Skill &skill : Skill::skills ) {
        if( skill.ident() == *this ) {
            return skill;
        }
    }

    const auto iter = Skill::contextual_skills.find( *this );
    if( iter != Skill::contextual_skills.end() ) {
        return iter->second;
    }

    return invalid_skill;
}

/** @relates string_id */
template<>
bool string_id<Skill>::is_valid() const
{
    return &obj() != &invalid_skill;
}

Skill::Skill() : Skill( skill_id::NULL_ID(), to_translation( "nothing" ),
                            to_translation( "The zen-most skill there is." ),
                            std::set<std::string> {}, skill_displayType_id::NULL_ID() )
{
}

Skill::Skill( const skill_id &ident, const translation &name, const translation &description,
              const std::set<std::string> &tags, skill_displayType_id display_type )
    : _ident( ident ), _name( name ), _description( description ), _tags( tags ),
      _display_type( display_type )
{
}

std::vector<const Skill *> Skill::get_skills_sorted_by(
    std::function<bool ( const Skill &, const Skill & )> pred )
{
    std::vector<const Skill *> result;
    result.reserve( skills.size() );

    for( const Skill &sk : skills ) {
        if( !sk.obsolete() ) {
            result.push_back( &sk );
        }
    }

    std::sort( begin( result ), end( result ), [&]( const Skill * lhs, const Skill * rhs ) {
        return pred( *lhs, *rhs );
    } );

    return result;
}

void Skill::reset()
{
    skills.clear();
    contextual_skills.clear();
}

void Skill::load_skill( const JsonObject &jsobj )
{
    // TEMPORARY until 0.G: Remove "ident" support
    skill_id ident = skill_id( jsobj.has_string( "ident" ) ? jsobj.get_string( "ident" ) :
                               jsobj.get_string( "id" ) );
    skills.erase( std::remove_if( begin( skills ), end( skills ), [&]( const Skill & s ) {
        return s._ident == ident;
    } ), end( skills ) );

    translation name;
    jsobj.read( "name", name );
    translation desc;
    jsobj.read( "description", desc );
    std::unordered_map<std::string, int> companion_skill_practice;
    for( JsonObject jo_csp : jsobj.get_array( "companion_skill_practice" ) ) {
        companion_skill_practice.emplace( jo_csp.get_string( "skill" ), jo_csp.get_int( "weight" ) );
    }
    time_info_t time_to_attack;
    if( jsobj.has_object( "time_to_attack" ) ) {
        JsonObject jso_tta = jsobj.get_object( "time_to_attack" );
        jso_tta.read( "min_time", time_to_attack.min_time );
        jso_tta.read( "base_time", time_to_attack.base_time );
        jso_tta.read( "time_reduction_per_level", time_to_attack.time_reduction_per_level );
    }
    skill_displayType_id display_type = skill_displayType_id( jsobj.get_string( "display_category" ) );
    Skill sk( ident, name, desc, jsobj.get_tags( "tags" ), display_type );

    sk._time_to_attack = time_to_attack;
    sk._companion_combat_rank_factor = jsobj.get_int( "companion_combat_rank_factor", 0 );
    sk._companion_survival_rank_factor = jsobj.get_int( "companion_survival_rank_factor", 0 );
    sk._companion_industry_rank_factor = jsobj.get_int( "companion_industry_rank_factor", 0 );
    sk._companion_skill_practice = companion_skill_practice;
    sk._obsolete = jsobj.get_bool( "obsolete", false );

    if( sk.is_contextual_skill() ) {
        contextual_skills[sk.ident()] = sk;
    } else {
        skills.push_back( sk );
    }
}

SkillDisplayType::SkillDisplayType() : SkillDisplayType( skill_displayType_id::NULL_ID(),
            to_translation( "invalid" ) )
{
}

SkillDisplayType::SkillDisplayType( const skill_displayType_id &ident,
                                    const translation &display_string )
    : _ident( ident ), _display_string( display_string )
{
}

void SkillDisplayType::load( const JsonObject &jsobj )
{
    // TEMPORARY until 0.G: Remove "ident" support
    skill_displayType_id ident = skill_displayType_id(
                                     jsobj.has_string( "ident" ) ? jsobj.get_string( "ident" ) :
                                     jsobj.get_string( "id" ) );
    skillTypes.erase( std::remove_if( begin( skillTypes ),
    end( skillTypes ), [&]( const SkillDisplayType & s ) {
        return s._ident == ident;
    } ), end( skillTypes ) );

    translation display_string;
    jsobj.read( "display_string", display_string );
    const SkillDisplayType sk( ident, display_string );
    skillTypes.push_back( sk );
}

const SkillDisplayType &SkillDisplayType::get_skill_type( const skill_displayType_id &id )
{
    for( auto &i : skillTypes ) {
        if( i._ident == id ) {
            return i;
        }
    }
    return invalid_skill_type;
}

skill_id Skill::from_legacy_int( const int legacy_id )
{
    static const std::array<skill_id, 28> legacy_skills = { {
            skill_id::NULL_ID(), skill_id( "dodge" ), skill_id( "melee" ), skill_id( "unarmed" ),
            skill_id( "bashing" ), skill_id( "cutting" ), skill_id( "stabbing" ), skill_id( "throw" ),
            skill_id( "gun" ), skill_id( "pistol" ), skill_id( "shotgun" ), skill_id( "smg" ),
            skill_id( "rifle" ), skill_id( "archery" ), skill_id( "launcher" ), skill_id( "mechanics" ),
            skill_id( "electronics" ), skill_id( "cooking" ), skill_id( "tailor" ), skill_id::NULL_ID(),
            skill_id( "firstaid" ), skill_id( "speech" ), skill_id( "computer" ),
            skill_id( "survival" ), skill_id( "traps" ), skill_id( "swimming" ), skill_id( "driving" ),
        }
    };
    if( static_cast<size_t>( legacy_id ) < legacy_skills.size() ) {
        return legacy_skills[legacy_id];
    }
    debugmsg( "legacy skill id %d is invalid", legacy_id );
    return skills.front().ident(); // return a non-null id because callers might not expect a null-id
}

skill_id Skill::random_skill()
{
    return random_entry_ref( skills ).ident();
}

// used for the pacifist trait
bool Skill::is_combat_skill() const
{
    static const std::string combat_skill( "combat_skill" );
    return _tags.count( combat_skill ) > 0;
}

bool Skill::is_contextual_skill() const
{
    static const std::string contextual_skill( "contextual_skill" );
    return _tags.count( contextual_skill ) > 0;
}

void SkillLevel::train( int amount, bool skip_scaling )
{
    // Working off rust to regain levels goes twice as fast as reaching levels in the first place
    if( _level < _highestLevel ) {
        amount *= 2;
    }

    if( skip_scaling ) {
        _exercise += amount;
    } else {
        const double scaling = get_option<float>( "SKILL_TRAINING_SPEED" );
        if( scaling > 0.0 ) {
            _exercise += roll_remainder( amount * scaling );
        }
    }

    if( _exercise >= 100 * 100 * ( _level + 1 ) * ( _level + 1 ) ) {
        _exercise = 0;
        ++_level;
        if( _level > _highestLevel ) {
            _highestLevel = _level;
        }
    }
}

namespace
{
time_duration rustRate( int level )
{
    // for n = [0, 7]
    //
    // 2^18
    // -------
    // 2^(18-n)

    unsigned const n = clamp( level, 0, 7 );
    return time_duration::from_turns( 1 << ( 18 - n ) );
}
} //namespace

bool SkillLevel::isRusting() const
{
    return get_option<std::string>( "SKILL_RUST" ) != "off" && ( _level > 0 ) &&
           calendar::turn - _lastPracticed > rustRate( _level );
}

bool SkillLevel::rust( bool charged_bio_mem, int character_rate )
{
    const time_duration delta = calendar::turn - _lastPracticed;
    const float char_rate = character_rate / 100.0f;
    const time_duration skill_rate = rustRate( _level );
    if( to_turns<int>( skill_rate ) * char_rate <= 0 || delta <= 0_turns ||
        delta % ( skill_rate * char_rate ) != 0_turns ) {
        return false;
    }

    if( charged_bio_mem ) {
        return one_in( 5 );
    }

    _exercise -= _level * 100;
    const std::string &rust_type = get_option<std::string>( "SKILL_RUST" );
    if( _exercise < 0 ) {
        if( rust_type == "vanilla" || rust_type == "int" ) {
            _exercise = ( 100 * 100 * _level * _level ) - 1;
            --_level;
        } else {
            _exercise = 0;
        }
    }

    return false;
}

void SkillLevel::practice()
{
    _lastPracticed = calendar::turn;
}

void SkillLevel::readBook( int minimumGain, int maximumGain, int maximumLevel )
{
    if( _level < maximumLevel || maximumLevel < 0 ) {
        train( ( _level + 1 ) * rng( minimumGain, maximumGain ) );
    }

    practice();
}

bool SkillLevel::can_train() const
{
    return get_option<float>( "SKILL_TRAINING_SPEED" ) > 0.0;
}

const SkillLevel &SkillLevelMap::get_skill_level_object( const skill_id &ident ) const
{
    static const SkillLevel null_skill{};

    if( ident && ident->is_contextual_skill() ) {
        debugmsg( "Skill \"%s\" is context-dependent.  It cannot be assigned.", ident.str() );
        return null_skill;
    }

    const auto iter = find( ident );

    if( iter != end() ) {
        return iter->second;
    }

    return null_skill;
}

SkillLevel &SkillLevelMap::get_skill_level_object( const skill_id &ident )
{
    static SkillLevel null_skill;

    if( ident && ident->is_contextual_skill() ) {
        debugmsg( "Skill \"%s\" is context-dependent.  It cannot be assigned.", ident.str() );
        return null_skill;
    }

    return ( *this )[ident];
}

void SkillLevelMap::mod_skill_level( const skill_id &ident, int delta )
{
    SkillLevel &obj = get_skill_level_object( ident );
    obj.level( obj.level() + delta );
}

int SkillLevelMap::get_skill_level( const skill_id &ident ) const
{
    return get_skill_level_object( ident ).level();
}

int SkillLevelMap::get_skill_level( const skill_id &ident, const item &context ) const
{
    const auto id = context.is_null() ? ident : context.contextualize_skill( ident );
    return get_skill_level( id );
}

bool SkillLevelMap::meets_skill_requirements( const std::map<skill_id, int> &req ) const
{
    return meets_skill_requirements( req, item() );
}

bool SkillLevelMap::meets_skill_requirements( const std::map<skill_id, int> &req,
        const item &context ) const
{
    return std::all_of( req.begin(), req.end(),
    [this, &context]( const std::pair<skill_id, int> &pr ) {
        return get_skill_level( pr.first, context ) >= pr.second;
    } );
}

std::map<skill_id, int> SkillLevelMap::compare_skill_requirements(
    const std::map<skill_id, int> &req ) const
{
    return compare_skill_requirements( req, item() );
}

std::map<skill_id, int> SkillLevelMap::compare_skill_requirements(
    const std::map<skill_id, int> &req, const item &context ) const
{
    std::map<skill_id, int> res;

    for( const auto &elem : req ) {
        const int diff = get_skill_level( elem.first, context ) - elem.second;
        if( diff != 0 ) {
            res[elem.first] = diff;
        }
    }

    return res;
}

int SkillLevelMap::exceeds_recipe_requirements( const recipe &rec ) const
{
    int over = rec.skill_used ? get_skill_level( rec.skill_used ) - rec.difficulty : 0;
    for( const auto &elem : compare_skill_requirements( rec.required_skills ) ) {
        over = std::min( over, elem.second );
    }
    return over;
}

bool SkillLevelMap::has_recipe_requirements( const recipe &rec ) const
{
    return exceeds_recipe_requirements( rec ) >= 0;
}

// Actually take the difference in social skill between the two parties involved
// Caps at 200% when you are 5 levels ahead, int comparison is handled in npctalk.cpp
double price_adjustment( int barter_skill )
{
    if( barter_skill <= 0 ) {
        return 1.0;
    }
    if( barter_skill >= 5 ) {
        return 2.0;
    }
    switch( barter_skill ) {
        case 1:
            return 1.05;
        case 2:
            return 1.15;
        case 3:
            return 1.30;
        case 4:
            return 1.65;
        default:
            // Should never occur
            return 1.0;
    }
}
