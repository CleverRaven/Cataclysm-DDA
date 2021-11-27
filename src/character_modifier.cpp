#include "character.h"
#include "move_mode.h"

static const limb_score_id limb_score_balance( "balance" );
static const limb_score_id limb_score_breathing( "breathing" );
static const limb_score_id limb_score_lift( "lift" );
static const limb_score_id limb_score_manip( "manip" );
static const limb_score_id limb_score_move_speed( "move_speed" );
static const limb_score_id limb_score_swim( "swim" );
static const limb_score_id limb_score_vision( "vision" );

static const skill_id skill_pistol( "pistol" );
static const skill_id skill_rifle( "rifle" );
static const skill_id skill_swimming( "swimming" );

// Scores

float Character::manipulator_score() const
{
    std::map<body_part_type::type, std::vector<bodypart>> bodypart_groups;
    std::vector<float> score_groups;
    for( const std::pair<const bodypart_str_id, bodypart> &id : body ) {
        bodypart_groups[id.first->limb_type].emplace_back( id.second );
    }
    for( std::pair<const body_part_type::type, std::vector<bodypart>> &part : bodypart_groups ) {
        float total = 0.0f;
        std::sort( part.second.begin(), part.second.end(),
        []( const bodypart & a, const bodypart & b ) {
            return a.get_limb_score_max( limb_score_manip ) < b.get_limb_score_max( limb_score_manip );
        } );
        for( const bodypart &id : part.second ) {
            total = std::min( total + id.get_limb_score( limb_score_manip ), id.get_limb_score_max( limb_score_manip ) );
        }
        score_groups.emplace_back( total );
    }
    const auto score_groups_max = std::max_element( score_groups.begin(), score_groups.end() );
    if( score_groups_max == score_groups.end() ) {
        return 0.0f;
    } else {
        return std::max( 0.0f, *score_groups_max );
    }
}

float Character::get_limb_score( const limb_score_id &score, const body_part_type::type &bp ) const
{
    int skill = -1;
    // manipulator/swim scores are treated a little special for now
    if( score == limb_score_manip ) {
        return manipulator_score();
    } else if( score == limb_score_swim ) {
        skill = get_skill_level( skill_swimming );
    }
    float total = 0.0f;
    for( const std::pair<const bodypart_str_id, bodypart> &id : body ) {
        if( bp == body_part_type::type::num_types || id.first->limb_type == bp ) {
            total += id.second.get_limb_score( score, skill );
        }
    }
    return std::max( 0.0f, total );
}

// Modifiers

double Character::aim_speed_skill_modifier( const skill_id &gun_skill ) const
{
    double skill_mult = 1.0;
    if( gun_skill == skill_pistol ) {
        skill_mult = 2.0;
    } else if( gun_skill == skill_rifle ) {
        skill_mult = 0.9;
    }
    /** @EFFECT_PISTOL increases aiming speed for pistols */
    /** @EFFECT_SMG increases aiming speed for SMGs */
    /** @EFFECT_RIFLE increases aiming speed for rifles */
    /** @EFFECT_SHOTGUN increases aiming speed for shotguns */
    /** @EFFECT_LAUNCHER increases aiming speed for launchers */
    return skill_mult * std::min( MAX_SKILL, get_skill_level( gun_skill ) );
}

double Character::aim_speed_dex_modifier() const
{
    return get_dex() - 8;
}

float Character::aim_speed_modifier() const
{
    return manipulator_score();
}

float Character::melee_thrown_move_modifier_hands() const
{
    if( manipulator_score() == 0.0f ) {
        return MAX_MOVECOST_MODIFIER;
    } else {
        return std::min( MAX_MOVECOST_MODIFIER, 1.0f / manipulator_score() );
    }
}

float Character::melee_thrown_move_modifier_torso() const
{
    float balance_score = get_limb_score( limb_score_balance );
    if( balance_score == 0.0f ) {
        return MAX_MOVECOST_MODIFIER;
    } else {
        return std::min( MAX_MOVECOST_MODIFIER, 1.0f / balance_score );
    }
}

float Character::melee_stamina_cost_modifier() const
{
    float lift_score = get_limb_score( limb_score_lift, body_part_type::type::arm );
    if( lift_score == 0.0f ) {
        return MAX_MOVECOST_MODIFIER;
    } else {
        return std::min( MAX_MOVECOST_MODIFIER, 1.0f / lift_score );
    }
}

float Character::reloading_move_modifier() const
{
    if( manipulator_score() == 0.0f ) {
        return MAX_MOVECOST_MODIFIER;
    } else {
        return std::min( MAX_MOVECOST_MODIFIER, 1.0f / manipulator_score() );
    }
}

float Character::thrown_dex_modifier() const
{
    return manipulator_score();
}

float Character::ranged_dispersion_modifier_hands() const
{
    if( manipulator_score() == 0.0f ) {
        return 1000.0f;
    } else {
        return std::min( 1000.0f,
                         ( 22.8f / manipulator_score() ) - 22.8f );
    }
}

float Character::ranged_dispersion_modifier_vision() const
{
    float vision_score = get_limb_score( limb_score_vision );
    if( vision_score == 0.0f ) {
        return 10'000.0f;
    } else {
        return std::min( 10'000.0f, ( 30.0f / vision_score ) - 30.0f );
    }
}

float Character::stamina_move_cost_modifier() const
{
    // Both walk and run speed drop to half their maximums as stamina approaches 0.
    // Convert stamina to a float first to allow for decimal place carrying
    float stamina_modifier = ( static_cast<float>( get_stamina() ) / get_stamina_max() + 1 ) / 2;
    return stamina_modifier * move_mode->move_speed_mult();
}

float Character::stamina_recovery_breathing_modifier() const
{
    return get_limb_score( limb_score_breathing );
}

float Character::limb_speed_movecost_modifier() const
{
    float move_speed_score = get_limb_score( limb_score_move_speed );
    if( move_speed_score == 0.0f ) {
        return MAX_MOVECOST_MODIFIER;
    } else {
        return std::min( MAX_MOVECOST_MODIFIER, 1.0f / move_speed_score );
    }
}

float Character::limb_footing_movecost_modifier() const
{
    float balance_score = get_limb_score( limb_score_balance );
    if( balance_score == 0.0f ) {
        return MAX_MOVECOST_MODIFIER;
    } else {
        return std::min( MAX_MOVECOST_MODIFIER, 1.0f / balance_score );
    }
}

float Character::limb_run_cost_modifier() const
{
    return ( limb_footing_movecost_modifier() + limb_speed_movecost_modifier() * 2 ) / 3.0f;
}

float Character::swim_modifier() const
{
    float swim_score = get_limb_score( limb_score_swim );
    if( swim_score == 0.0f ) {
        return MAX_MOVECOST_MODIFIER;
    } else {
        return std::min( MAX_MOVECOST_MODIFIER, 1.0f / swim_score );
    }
}

float Character::melee_attack_roll_modifier() const
{
    return std::max( 0.2f, get_limb_score( limb_score_balance ) );
}

