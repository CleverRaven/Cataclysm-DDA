#include <vector>
#include <utility>
#include <string>
#include <memory>
#include <map>

#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "character_attire.h"
#include "game.h"
#include "item.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "proficiency.h"
#include "test_data.h"
#include "type_id.h"

class Skill;

static const itype_id itype_debug_backpack( "debug_backpack" );

TEST_CASE( "monster_pulping_test" )
{
    clear_map();
    Character &you = get_player_character();
    for( const pulp_test_data &test : test_data::pulp_test ) {
        clear_character( you );
        // to reset character weight
        you.set_stored_kcal( 120000 );

        you.worn.wear_item( you, item( itype_debug_backpack ), false, false );

        for( const itype_id it : test.items ) {
            you.i_add( item( it ) );
        }
        for( const std::pair<const string_id<Skill>, int> &pair : test.skills ) {
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
                 p.stomps_only, p.weapon_only, p.can_cut_precisely );
        CHECK( test.expected_pulp_time == p.time_to_pulp );
    }
}
