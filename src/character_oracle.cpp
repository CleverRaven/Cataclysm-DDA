#include "character_oracle.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

#include "behavior.h"
#include "bodypart.h"
#include "character.h"
#include "item.h"
#include "itype.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "weather.h"

static const flag_id json_flag_FIRESTARTER( "FIRESTARTER" );

namespace behavior
{

// To avoid a local minima when the character has access to warmth in a shelter but gets cold
// when they go outside, this method needs to only alert when travel time to known shelter
// approaches time to freeze.
status_t character_oracle_t::needs_warmth_badly( std::string_view ) const
{
    // Use player::temp_conv to predict whether the Character is "in trouble".
    for( const bodypart_id &bp : subject->get_all_body_parts() ) {
        if( subject->get_part_temp_conv( bp ) <= BODYTEMP_VERY_COLD ) {
            return status_t::running;
        }
    }
    return status_t::success;
}

status_t character_oracle_t::needs_water_badly( std::string_view ) const
{
    // Check thirst threshold.
    if( subject->get_thirst() > 520 ) {
        return status_t::running;
    }
    return status_t::success;
}

status_t character_oracle_t::needs_food_badly( std::string_view ) const
{
    // Check hunger threshold.
    if( subject->get_hunger() >= 300 && subject->get_starvation() > base_metabolic_rate ) {
        return status_t::running;
    }
    return status_t::success;
}

status_t character_oracle_t::can_wear_warmer_clothes( std::string_view ) const
{
    // Check inventory for wearable warmer clothes, greedily.
    // Don't consider swapping clothes yet, just evaluate adding clothes.
    bool found_clothes = subject->has_item_with( [this]( const item & candidate ) {
        return candidate.get_warmth() > 0 && subject->can_wear( candidate ).success() &&
               !subject->is_worn( candidate );
    } );
    return found_clothes ? status_t::running : status_t::failure;
}

status_t character_oracle_t::can_make_fire( std::string_view ) const
{
    // Check inventory for firemaking tools and fuel
    bool tool = false;
    bool fuel = false;
    bool found_fire_stuff = subject->has_item_with( [&tool, &fuel]( const item & candidate ) {
        if( candidate.has_flag( json_flag_FIRESTARTER ) ) {
            tool = true;
            if( fuel ) {
                return true;
            }
        } else if( candidate.flammable() ) {
            fuel = true;
            if( tool ) {
                return true;
            }
        }
        return false;
    } );
    return found_fire_stuff ? status_t::running : status_t::success;
}

status_t character_oracle_t::can_take_shelter( std::string_view ) const
{
    // Stub: shelter detection not implemented yet. See #28681.
    return status_t::failure;
}

status_t character_oracle_t::has_water( std::string_view ) const
{
    // Check if we know about water somewhere
    bool found_water = subject->has_item_with( []( const item & cand ) {
        return cand.is_food() && cand.get_comestible()->quench > 0;
    } );
    return found_water ? status_t::running : status_t::failure;
}

status_t character_oracle_t::has_food( std::string_view ) const
{
    // Check if we know about food somewhere
    bool found_food = subject->has_item_with( []( const item & cand ) {
        return cand.is_food() && cand.get_comestible()->has_calories();
    } );
    return found_food ? status_t::running : status_t::failure;
}

float character_oracle_t::thirst_urgency( std::string_view ) const
{
    // 0 = hydrated, 1 = dehydration death (threshold 1200, character_health.cpp).
    static constexpr float death_threshold = 1200.0f;
    return std::clamp( subject->get_thirst() / death_threshold, 0.0f, 1.0f );
}

float character_oracle_t::hunger_urgency( std::string_view ) const
{
    // 0 = healthy weight, 1 = starvation death (stored_kcal <= 0, character_health.cpp).
    const int healthy = subject->get_healthy_kcal();
    if( healthy <= 0 ) {
        return 0.0f;
    }
    const float kcal_frac = static_cast<float>( subject->get_stored_kcal() ) / healthy;
    return std::clamp( 1.0f - kcal_frac, 0.0f, 1.0f );
}

float character_oracle_t::warmth_urgency( std::string_view ) const
{
    // 0 = all bodyparts at BODYTEMP_NORM (37C),
    // 1 = coldest bodypart at BODYTEMP_FREEZING (28C).
    const float norm = units::to_kelvin( BODYTEMP_NORM );
    const float freeze = units::to_kelvin( BODYTEMP_FREEZING );
    const float range = norm - freeze;
    if( range <= 0.0f ) {
        return 0.0f;
    }
    float coldest = norm;
    for( const bodypart_id &bp : subject->get_all_body_parts() ) {
        const float temp = units::to_kelvin( subject->get_part_temp_conv( bp ) );
        coldest = std::min( coldest, temp );
    }
    return std::clamp( ( norm - coldest ) / range, 0.0f, 1.0f );
}

} // namespace behavior
