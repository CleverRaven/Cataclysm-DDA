#include "character.h"
#include "map_helpers.h"
#include "cata_catch.h"
#include "game.h"
#include "player_helpers.h"
#include "coordinates.h"
#include "item.h"
#include "map.h"
#include "point.h"
#include "type_id.h"
#include "units.h"
#include "test_data.h"
#include "vpart_position.h"
#include "messages.h"
#include "calendar.h"

TEST_CASE( "monster_pulping_test" )
{
    clear_map();
    Character &you = get_player_character();
    for( pulp_test_data test : test_data::pulp_test ) {
        clear_character( you );
        // to reset character weight
        you.set_stored_kcal( 120000 );

        for( const itype_id it : test.items ) {
            you.i_add( item( it ) );
        }
        for( const std::pair<skill_id, int> &pair : test.skills ) {
            you.set_skill_level( pair.first, pair.second );
        }
        if( test.profs ) {
            for( const proficiency &prof : proficiency::get_all() ) {
                you.add_proficiency( prof.prof_id(), true, true );
            }
        }

        pulp_data p = g->calculate_pulpability( you, *test.corpse );

        std::string expected_pulp_speed = to_string( time_duration::from_seconds(
                                              test.expected_pulp_time ) );
        std::string actual_pulp_speed = to_string( time_duration::from_seconds(
                                            p.time_to_pulp ) );

        CAPTURE( test.name, expected_pulp_speed, actual_pulp_speed, p.bash_tool, p.pulp_power,
                 p.stomps_only, p.weapon_only, p.can_severe_cutting );
        CHECK( test.expected_pulp_time == p.time_to_pulp );
    }
}
