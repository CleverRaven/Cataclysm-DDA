#include "character_oracle.h"

#include <memory>
#include <vector>

#include "behavior.h"
#include "bodypart.h"
#include "character.h"
#include "item.h"
#include "itype.h"
#include "make_static.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "weather.h"

namespace behavior
{

// To avoid a local minima when the character has access to warmth in a shelter but gets cold
// when they go outside, this method needs to only alert when travel time to known shelter
// approaches time to freeze.
status_t character_oracle_t::needs_warmth_badly( const std::string_view ) const
{
    // Use player::temp_conv to predict whether the Character is "in trouble".
    for( const bodypart_id &bp : subject->get_all_body_parts() ) {
        if( subject->get_part_temp_conv( bp ) <= BODYTEMP_VERY_COLD ) {
            return status_t::running;
        }
    }
    return status_t::success;
}

status_t character_oracle_t::needs_water_badly( const std::string_view ) const
{
    // Check thirst threshold.
    if( subject->get_thirst() > 520 ) {
        return status_t::running;
    }
    return status_t::success;
}

status_t character_oracle_t::needs_food_badly( const std::string_view ) const
{
    // Check hunger threshold.
    if( subject->get_hunger() >= 300 && subject->get_starvation() > base_metabolic_rate ) {
        return status_t::running;
    }
    return status_t::success;
}

status_t character_oracle_t::can_wear_warmer_clothes( const std::string_view ) const
{
    // Check inventory for wearable warmer clothes, greedily.
    // Don't consider swapping clothes yet, just evaluate adding clothes.
    bool found_clothes = subject->has_item_with( [this]( const item & candidate ) {
        return candidate.get_warmth() > 0 && subject->can_wear( candidate ).success() &&
               !subject->is_worn( candidate );
    } );
    return found_clothes ? status_t::running : status_t::failure;
}

status_t character_oracle_t::can_make_fire( const std::string_view ) const
{
    // Check inventory for firemaking tools and fuel
    bool tool = false;
    bool fuel = false;
    bool found_fire_stuff = subject->has_item_with( [&tool, &fuel]( const item & candidate ) {
        if( candidate.has_flag( STATIC( flag_id( "FIRESTARTER" ) ) ) ) {
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

status_t character_oracle_t::can_take_shelter( const std::string_view ) const
{
    // There be no shelter here.
    // The frontline is everywhere.
    return status_t::failure;
}

status_t character_oracle_t::has_water( const std::string_view ) const
{
    // Check if we know about water somewhere
    bool found_water = subject->has_item_with( []( const item & cand ) {
        return cand.is_food() && cand.get_comestible()->quench > 0;
    } );
    return found_water ? status_t::running : status_t::failure;
}

status_t character_oracle_t::has_food( const std::string_view ) const
{
    // Check if we know about food somewhere
    bool found_food = subject->has_item_with( []( const item & cand ) {
        return cand.is_food() && cand.get_comestible()->has_calories();
    } );
    return found_food ? status_t::running : status_t::failure;
}

} // namespace behavior
