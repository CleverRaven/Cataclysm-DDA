#include "avatar.h"
#include "character_martial_arts.h"
#include "make_static.h"
#include "martialarts.h"
#include "mutation.h"
#include "options.h"
#include "player_difficulty.h"
#include "profession.h"
#include "skill.h"

static const damage_type_id damage_bash( "bash" );

static const profession_id profession_unemployed( "unemployed" );

player_difficulty::player_difficulty()
{
    // set up an average NPC
    average = npc();
    reset_npc( average );
    average.prof = &profession_unemployed.obj();
}

// creates an npc with similar stats to an avatar for testing
void player_difficulty::npc_from_avatar( const avatar &u, npc &dummy )
{
    // set stats
    dummy.str_max = u.str_max;
    dummy.dex_max = u.dex_max;
    dummy.int_max = u.int_max;
    dummy.per_max = u.per_max;

    // set skills
    for( const auto &t : u.get_all_skills() ) {
        dummy.set_skill_level( t.first, t.second.level() );
    }

    // set profession and hobbies
    dummy.prof = u.prof;
    dummy.hobbies = u.hobbies;

    // set mutations
    for( const trait_id &t : u.get_mutations( true ) ) {
        dummy.set_mutation( t );
    }

    dummy.reset();
    dummy.initialize( false );
}

void player_difficulty::reset_npc( Character &dummy )
{
    dummy.set_body();
    dummy.normalize(); // In particular this clears martial arts style

    // delete all worn items.
    dummy.clear_worn();
    dummy.calc_encumbrance();
    dummy.inv->clear();
    dummy.remove_weapon();
    dummy.clear_mutations();

    // Clear stomach and then eat a nutritious meal to normalize stomach
    // contents (needs to happen before clear_morale).
    dummy.stomach.empty();
    dummy.guts.empty();
    dummy.clear_vitamins();

    // This sets HP to max, clears addictions and morale,
    // and sets hunger, thirst, fatigue and such to zero
    dummy.environmental_revert_effect();
    // However, the above does not set stored kcal
    dummy.set_stored_kcal( dummy.get_healthy_kcal() );

    dummy.empty_skills();
    dummy.martial_arts_data->clear_styles();
    dummy.clear_morale();
    dummy.clear_bionics();
    dummy.activity.set_to_null();
    dummy.reset_chargen_attributes();
    dummy.set_pain( 0 );
    dummy.reset_bonuses();
    dummy.set_speed_base( 100 );
    dummy.set_speed_bonus( 0 );
    dummy.set_sleep_deprivation( 0 );
    for( const proficiency_id &prof : dummy.known_proficiencies() ) {
        dummy.lose_proficiency( prof, true );
    }

    // Reset cardio_acc to baseline
    dummy.reset_cardio_acc();
    // Restore all stamina and go to walk mode
    dummy.set_stamina( dummy.get_stamina_max() );
    dummy.reset_activity_level();

    // Make sure we don't carry around weird effects.
    dummy.clear_effects();

    // Make stats nominal.
    dummy.str_max = 8;
    dummy.dex_max = 8;
    dummy.int_max = 8;
    dummy.per_max = 8;
    dummy.set_str_bonus( 0 );
    dummy.set_dex_bonus( 0 );
    dummy.set_int_bonus( 0 );
    dummy.set_per_bonus( 0 );
}

double player_difficulty::calc_armor_value( const Character &u )
{
    // a low damage value to be concerned about early
    // only concerned with bash damage since it is most
    // prevalent early game

    const float head_protection = 0.2f;
    const float torso_protection = 0.4f;
    const float arms_protection = 0.2f;
    const float legs_protection = 0.2f;

    // don't want divide by zeroes later
    float armor_val = 0.01f;

    // check any other items the character has on them
    if( u.prof ) {
        for( const item &i : u.prof->items( true, std::vector<trait_id>() ) ) {
            armor_val += head_protection * i.resist( damage_bash, false, bodypart_id( "head" ) );
            armor_val += torso_protection * i.resist( damage_bash, false, bodypart_id( "torso" ) );
            armor_val += arms_protection * i.resist( damage_bash, false, bodypart_id( "arm_r" ) );
            armor_val += legs_protection * i.resist( damage_bash, false, bodypart_id( "leg_r" ) );
        }
    }

    return armor_val;
}

std::string player_difficulty::get_defense_difficulty( const Character &u ) const
{
    // figure out how survivable the character is.

    // the percent margin between result bands
    const float percent_band = 0.5f;

    // how much each portion is valued compared to the deviation of others
    // movement is negative since lower is better
    // small deviations in movement value leads to massive defense value
    const float standard_movement = -6.0f;
    const float difficult_movement = -2.0f;
    const float dodge = 1.0f;
    const float armor_value = 0.2f;

    // calculate move cost on simple ground
    float player_run_cost = static_cast<float>( u.run_cost( 100 ) ) / static_cast<float>
                            ( u.get_speed() );
    float average_run_cost = static_cast<float>( average.run_cost( 100 ) ) / static_cast<float>
                             ( average.get_speed() );
    float per = standard_movement * ( player_run_cost - average_run_cost ) / average_run_cost;

    // calculate move cost on simple ground
    player_run_cost = static_cast<float>( u.run_cost( 400 ) ) / static_cast<float>
                      ( u.get_speed() );
    average_run_cost = static_cast<float>( average.run_cost( 400 ) ) / static_cast<float>
                       ( average.get_speed() );
    per += difficult_movement * ( player_run_cost - average_run_cost ) / average_run_cost;

    // calculate armor value
    float player_armor = calc_armor_value( u );
    float average_armor = calc_armor_value( average );
    per += armor_value * ( player_armor - average_armor ) / average_armor;
    // calculate dodge
    per += dodge * ( u.get_dodge() - average.get_dodge() ) / average.get_dodge();

    return format_output( percent_band, per );
}

double player_difficulty::calc_dps_value( const Character &u )
{
    // check against the big three
    // efficient early weapons you can easily get access to
    item early_piercing = item( "knife_combat" );
    item early_cutting = item( "machete" );
    item early_bashing = item( "bat" );

    double baseline = std::max( u.weapon_value( early_piercing ),
                                u.weapon_value( early_cutting ) );
    baseline = std::max( baseline, u.weapon_value( early_bashing ) );

    // check any other items the character has on them
    if( u.prof ) {
        for( const item &i : u.prof->items( true, std::vector<trait_id>() ) ) {
            baseline = std::max( baseline, u.weapon_value( i ) );
        }
    }

    return baseline;
}

std::string player_difficulty::get_combat_difficulty( const Character &u ) const
{
    // figure out how good the player is at combat.
    // get the best dps of a basic civilian with 3 scavengable weapons
    // compare to the dps of this character with 1 of those 3 or their best weapon they spawn with

    // the percent margin between result bands
    const float percent_band = 0.5f;

    // dps should be multiplied by speed which is a multiplier
    double npc_dps = calc_dps_value( average ) * average.get_speed();
    double player_dps = calc_dps_value( u ) * u.get_speed();

    float per = ( player_dps - npc_dps ) / npc_dps;

    return format_output( percent_band, per );
}

std::string player_difficulty::get_genetics_difficulty( const Character &u ) const
{
    // figure out how genetically advantaged the character is

    // how many attributes the average character has
    const int average_stats = 38;
    // how much stats above HIGH_STAT penalize the player
    const int high_stat_penalty = 4;
    // how much traits are valued at
    const int trait_value = 1;
    // the percent margin between result bands
    const float percent_band = 2.5f;

    int genetics_total = u.str_max + u.dex_max + u.per_max + u.int_max;
    genetics_total += std::max( 0, u.str_max - HIGH_STAT ) * high_stat_penalty;
    genetics_total += std::max( 0, u.dex_max - HIGH_STAT ) * high_stat_penalty;
    genetics_total += std::max( 0, u.per_max - HIGH_STAT ) * high_stat_penalty;
    genetics_total += std::max( 0, u.int_max - HIGH_STAT ) * high_stat_penalty;

    // each trait effects genetics slightly as well
    for( const trait_id &trait : u.get_mutations( true ) ) {
        if( trait->points > 0 ) {
            genetics_total += trait_value;
        } else if( trait->points < 0 ) {
            genetics_total += -1 * trait_value;
        }
    }

    // note, not doing a percent calc here just actually evaluating closeness average
    // we subtract percent band from the average since "average" in the menu goes from
    // 0 to 1 percent band
    float per = static_cast<float>( genetics_total - ( average_stats - percent_band ) );

    return format_output( percent_band, per );
}

std::string player_difficulty::get_expertise_difficulty( const Character &u ) const
{
    // a bit extra over multipool since you get 2 points per in multi
    const int average_skill_ranks = 4;
    const float percent_band = 0.6f;

    // how much each proficiency is valued compared to a skill point
    const int proficiency_value = 2;

    // how much each portion is valued compared to the deviation of others
    const float skill_weighting = 1.0f;
    const float reading_weighting = 2.0f;
    const float learn_weighting = 2.0f;
    const float focus_weighting = 2.0f;

    // sum player skills and proficiencies
    int player_skills = 0;
    // every skill point is worth 1 point of value
    for( const auto &t : u.get_all_skills() ) {
        // combat skills will be handled in offence
        if( !t.first->is_combat_skill() ) {
            player_skills += t.second.level();
        }
    }
    // every proficiency is worth about the value of 2 skill points
    player_skills += proficiency_value * u._proficiencies->known_profs().size();

    // skills and professions
    float per = skill_weighting * static_cast<float>( player_skills - average_skill_ranks ) /
                static_cast<float>( average_skill_ranks );

    // focus
    per += focus_weighting * static_cast<float>( u.calc_focus_equilibrium(
                true ) - average.calc_focus_equilibrium(
                true ) ) / static_cast<float>( average.calc_focus_equilibrium( true ) );

    // reading speed negative is good
    per -= reading_weighting * static_cast<float>( u.read_speed() - average.read_speed() ) /
           static_cast<float>( average.read_speed() );

    // how much each point of experience is worth to your character
    per += learn_weighting * static_cast<float>( u.adjust_for_focus( 100 ) -
            average.adjust_for_focus( 100 ) ) / static_cast<float>( average.adjust_for_focus( 100 ) );

    return format_output( percent_band, per );
}

int player_difficulty::calc_social_value( const Character &u, const npc &compare )
{
    // weighting skill and underlying values equally

    int social = std::max( u.intimidation(), u.persuade_skill() );
    int lying = u.lie_skill();

    npc_opinion npc_opinion = compare.get_opinion_values( u );

    int oppinion = npc_opinion.trust;
    oppinion -= npc_opinion.anger;
    oppinion += npc_opinion.fear;
    oppinion += npc_opinion.value;

    return social + lying + oppinion;
}

std::string player_difficulty::get_social_difficulty( const Character &u ) const
{
    // compare the characters social value to an average npc
    int player_val = calc_social_value( u, average );
    int average_val = calc_social_value( average, average );

    const float percent_band = 0.6f;

    float per = static_cast<float>( player_val - average_val ) / static_cast<float>
                ( average_val );

    return format_output( percent_band, per );
}

std::string player_difficulty::format_output( float percent_band, float per )
{
    std::string output;
    if( per < -1 * percent_band ) {
        output = string_format( "<color_%s>%s</color>", "light_red", _( "underpowered" ) );
    } else if( per < 0.0f ) {
        output = string_format( "<color_%s>%s</color>", "light_red", _( "weak" ) );
    } else if( per < percent_band ) {
        output = string_format( "<color_%s>%s</color>", "yellow", _( "average" ) );
    } else if( per < 2 * percent_band ) {
        output = string_format( "<color_%s>%s</color>", "light_green", _( "strong" ) );
    } else if( per < 3 * percent_band ) {
        output = string_format( "<color_%s>%s</color>", "light_green", _( "powerful" ) );
    } else {
        output = string_format( "<color_%s>%s</color>", "light_green", _( "overpowered" ) );
    }

    if( get_option<bool>( "DEBUG_DIFFICULTIES" ) ) {
        output = string_format( "%2f: %s", per, output );
    }
    return output;
}

const npc &player_difficulty::get_average_npc()
{
    return average;
}

std::string player_difficulty::difficulty_to_string( const avatar &u ) const
{
    // make a faux avatar that can have the effects of creation applied to it
    npc n = npc();
    reset_npc( n );
    npc_from_avatar( u, n );

    std::string genetics = get_genetics_difficulty( n );
    std::string socials = get_social_difficulty( n );
    std::string expertise = get_expertise_difficulty( n );
    std::string combat = get_combat_difficulty( n );
    std::string defense = get_defense_difficulty( n );

    return string_format( "%s |  %s: %s  %s: %s  %s: %s  %s: %s  %s: %s",
                          _( "Summary" ),
                          _( "Lifestyle" ), genetics,
                          _( "Knowledge" ), expertise,
                          _( "Offense" ), combat,
                          _( "Defense" ), defense,
                          _( "Social" ), socials );
}
