#include "behavior_oracle.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <utility>

#include "behavior.h"
#include "character_oracle.h"
#include "monster_oracle.h"

namespace behavior
{

status_t return_running( const oracle_t *, std::string_view )
{
    return status_t::running;
}

// Just a little helper to make populating predicate_map slightly less gross.
static std::function < status_t( const oracle_t *, std::string_view ) >
make_function( status_t ( character_oracle_t::* fun )( std::string_view ) const )
{
    return static_cast<status_t ( oracle_t::* )( std::string_view ) const>( fun );
}

static std::function < status_t( const oracle_t *, std::string_view ) >
make_function( status_t ( monster_oracle_t::* fun )( std::string_view ) const )
{
    return static_cast<status_t ( oracle_t::* )( std::string_view ) const>( fun );
}

static std::function<float( const oracle_t *, std::string_view )>
make_score_function( float ( character_oracle_t::* fun )( std::string_view ) const )
{
    return static_cast<float ( oracle_t::* )( std::string_view ) const>( fun );
}

std::unordered_map<std::string, std::function<status_t( const oracle_t *, std::string_view ) >>
predicate_map = {{
        { "npc_needs_warmth_badly", make_function( &character_oracle_t::needs_warmth_badly ) },
        { "npc_needs_water_badly", make_function( &character_oracle_t::needs_water_badly ) },
        { "npc_needs_food_badly", make_function( &character_oracle_t::needs_food_badly ) },
        { "npc_can_wear_warmer_clothes", make_function( &character_oracle_t::can_wear_warmer_clothes ) },
        { "npc_can_make_fire", make_function( &character_oracle_t::can_make_fire ) },
        { "npc_can_take_shelter", make_function( &character_oracle_t::can_take_shelter ) },
        { "npc_has_water", make_function( &character_oracle_t::has_water ) },
        { "npc_has_food", make_function( &character_oracle_t::has_food ) },
        { "npc_can_obtain_food", make_function( &character_oracle_t::can_obtain_food ) },
        { "npc_can_obtain_water", make_function( &character_oracle_t::can_obtain_water ) },
        { "npc_can_obtain_warmth", make_function( &character_oracle_t::can_obtain_warmth ) },
        { "npc_needs_sleep_badly", make_function( &character_oracle_t::needs_sleep_badly ) },
        { "npc_can_sleep", make_function( &character_oracle_t::can_sleep ) },
        { "npc_in_danger", make_function( &character_oracle_t::in_danger ) },
        { "npc_should_flee", make_function( &character_oracle_t::should_flee ) },
        { "npc_has_target", make_function( &character_oracle_t::has_target ) },
        { "npc_has_sound_alerts", make_function( &character_oracle_t::has_sound_alerts ) },
        { "npc_displaced_from_post", make_function( &character_oracle_t::displaced_from_post ) },
        { "npc_on_shift", make_function( &character_oracle_t::on_shift ) },
        { "npc_is_following", make_function( &character_oracle_t::npc_is_following ) },
        { "npc_should_embark", make_function( &character_oracle_t::npc_should_embark ) },
        { "npc_has_goto_order", make_function( &character_oracle_t::npc_has_goto_order ) },
        { "npc_has_camp_job", make_function( &character_oracle_t::has_camp_job ) },
        { "npc_is_away_from_camp", make_function( &character_oracle_t::is_away_from_camp ) },
        { "npc_is_camp_idle", make_function( &character_oracle_t::is_camp_idle ) },
        { "monster_not_hallucination", make_function( &monster_oracle_t::not_hallucination ) },
        { "monster_items_available", make_function( &monster_oracle_t::items_available ) },
        { "monster_split_possible", make_function( &monster_oracle_t::split_possible ) },
        { "monster_adjacent_plants", make_function( &monster_oracle_t::adjacent_plants ) },
        { "monster_special_available", make_function( &monster_oracle_t::special_available ) }
    }
};

std::unordered_map<std::string, std::function<float( const oracle_t *, std::string_view )>>
score_predicate_map = {{
        { "npc_thirst_urgency", make_score_function( &character_oracle_t::thirst_urgency ) },
        { "npc_hunger_urgency", make_score_function( &character_oracle_t::hunger_urgency ) },
        { "npc_warmth_urgency", make_score_function( &character_oracle_t::warmth_urgency ) },
        { "npc_sleepiness_urgency", make_score_function( &character_oracle_t::sleepiness_urgency ) },
        { "npc_duty_urgency", make_score_function( &character_oracle_t::duty_urgency ) },
        { "npc_following_urgency", make_score_function( &character_oracle_t::npc_following_urgency ) },
        { "npc_embark_urgency", make_score_function( &character_oracle_t::npc_embark_urgency ) },
        { "npc_goto_order_urgency", make_score_function( &character_oracle_t::npc_goto_order_urgency ) },
        { "npc_camp_work_urgency", make_score_function( &character_oracle_t::camp_work_urgency ) },
        { "npc_return_to_camp_urgency", make_score_function( &character_oracle_t::return_to_camp_urgency ) },
        { "npc_free_time_urgency", make_score_function( &character_oracle_t::free_time_urgency ) }
    }
};

} // namespace behavior
