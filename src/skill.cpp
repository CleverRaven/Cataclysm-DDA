#include "skill.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <utility>

#include "cata_utility.h"
#include "debug.h"
#include "game_constants.h"
#include "item.h"
#include "json.h"
#include "options.h"
#include "recipe.h"
#include "rng.h"
#include "translations.h"

static const skill_id skill_archery( "archery" );
static const skill_id skill_bashing( "bashing" );
static const skill_id skill_computer( "computer" );
static const skill_id skill_cooking( "cooking" );
static const skill_id skill_cutting( "cutting" );
static const skill_id skill_dodge( "dodge" );
static const skill_id skill_driving( "driving" );
static const skill_id skill_electronics( "electronics" );
static const skill_id skill_firstaid( "firstaid" );
static const skill_id skill_gun( "gun" );
static const skill_id skill_launcher( "launcher" );
static const skill_id skill_mechanics( "mechanics" );
static const skill_id skill_melee( "melee" );
static const skill_id skill_pistol( "pistol" );
static const skill_id skill_rifle( "rifle" );
static const skill_id skill_shotgun( "shotgun" );
static const skill_id skill_smg( "smg" );
static const skill_id skill_speech( "speech" );
static const skill_id skill_stabbing( "stabbing" );
static const skill_id skill_survival( "survival" );
static const skill_id skill_swimming( "swimming" );
static const skill_id skill_tailor( "tailor" );
static const skill_id skill_throw( "throw" );
static const skill_id skill_traps( "traps" );
static const skill_id skill_unarmed( "unarmed" );

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
    if( jsobj.has_int( "sort_rank" ) ) {
        sk._sort_rank = jsobj.get_int( "sort_rank" );
    } else {
        sk._sort_rank = 1000000;
        debugmsg( "skill '%s' missing 'sort_rank' field.", ident.str() );
    }

    sk._time_to_attack = time_to_attack;
    sk._companion_combat_rank_factor = jsobj.get_int( "companion_combat_rank_factor", 0 );
    sk._companion_survival_rank_factor = jsobj.get_int( "companion_survival_rank_factor", 0 );
    sk._companion_industry_rank_factor = jsobj.get_int( "companion_industry_rank_factor", 0 );
    sk._companion_skill_practice = companion_skill_practice;
    sk._obsolete = jsobj.get_bool( "obsolete", false );
    sk._teachable = jsobj.get_bool( "teachable", true );

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

/** @relates string_id */
template<>
const SkillDisplayType &skill_displayType_id::obj() const
{
    for( const SkillDisplayType &skill : SkillDisplayType::skillTypes ) {
        if( skill.ident() == *this ) {
            return skill;
        }
    }

    return invalid_skill_type;
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
    for( SkillDisplayType &i : skillTypes ) {
        if( i._ident == id ) {
            return i;
        }
    }
    return invalid_skill_type;
}

skill_id Skill::from_legacy_int( const int legacy_id )
{
    static const std::array<skill_id, 28> legacy_skills = { {
            skill_id::NULL_ID(), skill_dodge, skill_melee, skill_unarmed,
            skill_bashing, skill_cutting, skill_stabbing, skill_throw,
            skill_gun, skill_pistol, skill_shotgun, skill_smg,
            skill_rifle, skill_archery, skill_launcher, skill_mechanics,
            skill_electronics, skill_cooking, skill_tailor, skill_id::NULL_ID(),
            skill_firstaid, skill_speech, skill_computer,
            skill_survival, skill_traps, skill_swimming, skill_driving,
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

void SkillLevel::train( int amount, float catchup_modifier, float knowledge_modifier,
                        bool allow_multilevel )
{
    if( amount < 0 ) {
        debugmsg( "train() called with negative xp: %d", amount );
        return;
    }
    // catchup gets faster the higher the level gap gets.
    float level_gap = 1.0f * std::max( knowledgeLevel(), 1 ) / std::max( level(), 1 );
    float catchup_amount = amount * catchup_modifier;
    float knowledge_amount = amount * knowledge_modifier;
    if( knowledgeLevel() > level() ) {
        catchup_amount *= level_gap;
    } else if( knowledgeLevel() == level() && _knowledgeExperience > _exercise ) {
        // when you're in the same level, the catchup starts to slow down.
        catchup_amount = amount * std::max( catchup_modifier - 1.0f * _exercise / _knowledgeExperience,
                                            1.0f );
        knowledge_amount = amount * std::max( knowledge_modifier - 0.1f * _exercise / _knowledgeExperience,
                                              1.0f );
    } else {
        // When your two xp's are equal just do the basic thing.
        catchup_amount = amount * 1.0f;
        knowledge_amount = amount * 1.0f;
    }

    // Learning knowledge faster than skill, when you're actually practicing, will generate some annoying problems.
    if( knowledge_amount > catchup_amount * 0.9f ) {
        knowledge_amount = catchup_amount * 0.9f;
    }

    if( unadjustedKnowledgeLevel() >= MAX_SKILL ) {
        knowledge_amount = 0;
    }

    const double scaling = get_option<float>( "SKILL_TRAINING_SPEED" );
    if( scaling > 0.0 ) {
        catchup_amount *= scaling;
        knowledge_amount *= scaling;
    }
    _exercise += catchup_amount;
    if( _exercise < 0 ) {
        debugmsg( "integer overflow in train() amount=%d catchup_modifier=%g knowledge_modifier=%g level_gap=%g catchup_amount=%g knowledge_amount=%g scaling=%g _exercise=%d",
                  amount, catchup_modifier, knowledge_modifier, level_gap, catchup_amount, knowledge_amount, scaling,
                  _exercise );
        _exercise -= catchup_amount;
        return;
    }
    _rustAccumulator -= catchup_amount;
    _knowledgeExperience += knowledge_amount;

    on_exercise_change( allow_multilevel );
    practice();
}

void SkillLevel::knowledge_train( int amount, int npc_knowledge )
{
    float level_gap = 1.0f;
    // when your _level is the same or 1 level below your knowledge, gain xp at the normal rate.
    // as your practical skill lags behind your knowledge, it gets harder to contextualize that
    // theoretical knowledge, and your ability to learn the theory gets slower.

    // The same formula applies to NPCs teaching you, but in that case the level decreases as their knowledge
    // level exceeds your own.  The best teacher is one who is only somewhat more knowledgeable than you.
    if( npc_knowledge > 0 ) {
        // This should later be modified by NPC teaching proficiencies.
        level_gap = std::max<float>( npc_knowledge - unadjustedKnowledgeLevel(), 1.0f );
    } else {
        // Some day this should be affected by json specific to the skill, some skills are more amenable
        // to book learning.
        level_gap = std::max<float>( unadjustedKnowledgeLevel() - unadjustedLevel(), 1.0f );
    }
    float level_mult = 2.0f / ( level_gap + 1.0f );
    amount *= level_mult;

    const double scaling = get_option<float>( "SKILL_TRAINING_SPEED" );
    if( scaling > 0.0 ) {
        amount = std::ceil( amount * scaling );
    }
    _knowledgeExperience += amount;

    if( _knowledgeExperience >= 10000 * pow( unadjustedKnowledgeLevel() + 1, 2U ) ) {
        _knowledgeExperience = 0;
        ++_knowledgeLevel;
    }

}

bool SkillLevel::isRusty() const
{
    // skill is considered rusty if the practical xp lags knowledge xp by at least 1%
    return level() != knowledgeLevel() ||
           _knowledgeExperience - _exercise >= pow( level() + 1, 2U ) * 10;
}

bool SkillLevel::rust( int rust_resist, float rust_multiplier )
{
    if( ( calendar::turn - _lastPracticed ) < 24_hours ) {
        // don't rust within the grace period
        return false;
    }

    if( unadjustedLevel() >= MAX_SKILL ) {
        // don't rust any more once you hit the level cap, at least until we have a way to "pause" rust for a while.
        return false;
    }

    const int level_multiplier = pow( unadjustedLevel() + 1, 2U );
    float level_exp = level_multiplier * 10000.0f;
    if( _rustAccumulator > level_exp * 3 ) {
        // at this point the numbers ahead will be too small to bother.  Just cap it off.
        return false;
    }

    // Future plans: Have rust_slowdown impacted by intelligence and other memory-affecting things
    float rust_slowdown = std::max( static_cast<float>( std::sqrt( _rustAccumulator / level_exp ) ),
                                    0.04f );

    // rust amount starts at 4% of a level's xp, run every 24 hours.
    // Once the accumulated rust exceeds 16% of a level, rust_amount starts to drop.
    int rust_amount = level_multiplier * rust_multiplier * 16 / rust_slowdown;

    if( rust_resist > 0 ) {
        rust_amount = std::lround( rust_amount * ( std::max( ( 100 - rust_resist ), 0 ) / 100.0 ) );
    }

    if( level() == 0 ) {
        rust_amount = std::min( rust_amount, _exercise );
    }

    if( rust_amount <= 0 ) {
        return false;
    }

    _rustAccumulator += rust_amount;
    on_exercise_change();

    return false;
}

void SkillLevel::practice()
{
    _lastPracticed = calendar::turn;
}

void SkillLevel::readBook( int minimumGain, int maximumGain, int maximumLevel )
{
    if( knowledgeLevel() < maximumLevel || maximumLevel < 0 ) {
        knowledge_train( ( knowledgeLevel() + 1 ) * rng( minimumGain, maximumGain ) * 100 );
    }

    practice();
}

bool SkillLevel::can_train() const
{
    return get_option<float>( "SKILL_TRAINING_SPEED" ) > 0.0;
}

void SkillLevel::set_exercise( int value, bool raw )
{
    _exercise = raw ? value : value * ( 100 * ( level() + 1 ) * ( level() + 1 ) );
    on_exercise_change( true );
}

void SkillLevel::on_exercise_change( bool allow_multilevel )
{
    if( _exercise < 0 ) {
        _exercise = ( 100 * 100 * pow( unadjustedLevel(), 2U ) ) - 1;
        --_level;
    } else {
        const auto xp_to_level = [&]() {
            return 100 * 100 * pow( unadjustedLevel() + 1, 2U );
        };
        while( _exercise >= xp_to_level() ) {
            _exercise = allow_multilevel ? _exercise - xp_to_level() : 0;
            ++_level;
            if( unadjustedLevel() > unadjustedKnowledgeLevel() ) {
                _knowledgeLevel = _level;
                _knowledgeExperience = 0;
            }
        }

        if( _rustAccumulator < 0 ) {
            _rustAccumulator = 0;
        }
        if( level() == knowledgeLevel() && _exercise > _knowledgeExperience ) {
            _knowledgeExperience = _exercise;
        }

        if( _knowledgeExperience >= 10000 * pow( unadjustedKnowledgeLevel() + 1, 2U ) ) {
            _knowledgeExperience = 0;
            ++_knowledgeLevel;
        }
    }
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

void SkillLevelMap::mod_knowledge_level( const skill_id &ident, int delta )
{
    SkillLevel &obj = get_skill_level_object( ident );
    obj.knowledgeLevel( obj.knowledgeLevel() + delta );
}

int SkillLevelMap::get_skill_level( const skill_id &ident ) const
{
    return get_skill_level_object( ident ).level();
}

int SkillLevelMap::get_skill_level( const skill_id &ident, const item &context ) const
{
    const skill_id id = context.is_null() ? ident : context.contextualize_skill( ident );
    return get_skill_level( id );
}

float SkillLevelMap::get_progress_level( const skill_id &ident ) const
{
    return static_cast<float>( get_skill_level_object( ident ).exercise() ) / 100.0f;
}

float SkillLevelMap::get_progress_level( const skill_id &ident, const item &context ) const
{
    const skill_id id = context.is_null() ? ident : context.contextualize_skill( ident );
    return get_progress_level( id );
}

int SkillLevelMap::get_knowledge_level( const skill_id &ident ) const
{
    return get_skill_level_object( ident ).knowledgeLevel();
}

int SkillLevelMap::get_knowledge_level( const skill_id &ident, const item &context ) const
{
    const skill_id id = context.is_null() ? ident : context.contextualize_skill( ident );
    return get_knowledge_level( id );
}

float SkillLevelMap::get_knowledge_progress_level( const skill_id &ident ) const
{
    return static_cast<float>( get_skill_level_object( ident ).knowledgeExperience() ) / 100.0f;
}

float SkillLevelMap::get_knowledge_progress_level( const skill_id &ident,
        const item &context ) const
{
    const skill_id id = context.is_null() ? ident : context.contextualize_skill( ident );
    return get_progress_level( id );
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
        // Whether or not you meet skill requirements should be based on your level of theory training,
        // not practical experience.
        return get_knowledge_level( pr.first, context ) >= pr.second;
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

bool SkillLevelMap::theoretical_recipe_requirements( const recipe &rec ) const
{
    // Regardless of your current practical skill, do you know the theory of how to make this thing?
    if( rec.skill_used && get_knowledge_level( rec.skill_used ) < rec.difficulty ) {
        return false;
    }
    for( const std::pair<const skill_id, int> &elem : rec.required_skills ) {
        if( get_knowledge_level( elem.first ) < elem.second ) {
            return false;
        }
    }
    return true;
}

bool SkillLevelMap::has_recipe_requirements( const recipe &rec ) const
{
    return exceeds_recipe_requirements( rec ) >= 0 || theoretical_recipe_requirements( rec );
}

bool SkillLevelMap::has_same_levels_as( const SkillLevelMap &other ) const
{
    if( this->size() != other.size() ) {
        return false;
    }
    for( const auto &entry : *this ) {
        const SkillLevel &this_level = entry.second;
        if( other.count( entry.first ) == 0 ) {
            return false;
        }
        const SkillLevel &other_level = other.get_skill_level_object( entry.first );
        if( this_level.level() != other_level.level() ||
            this_level.knowledgeLevel() != other_level.knowledgeLevel() ) {
            return false;
        }
    }
    return true;
}

// Actually take the difference in social skill between the two parties involved
// Caps at 200% when you are 5 levels ahead, int comparison is handled in npctalk.cpp
double price_adjustment( int barter_skill )
{
    int const skill = std::min( 5, std::abs( barter_skill ) );
    double const val = 0.045 * std::pow( skill, 2 ) - 0.025 * skill + 1;
    return barter_skill >= 0 ? val : 1 / val;
}
