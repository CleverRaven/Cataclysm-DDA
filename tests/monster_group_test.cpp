#include "catch/catch.hpp"

#include "enums.h"
#include "monster_group.h"
#include "monster.h"
#include "senses.h"

#include <map>


class mock_senses : public senses {
public:
    mock_senses( tripoint scent_dest, tripoint vis_dest ) :
        scent_destination(scent_dest), vision_destination(vis_dest) {}

    mock_senses( tripoint scent_dest, tripoint vis_dest, std::set<tripoint> &blocked_squares ) :
        scent_destination(scent_dest), vision_destination(vis_dest),
        non_navigable_squares(blocked_squares) {}

    tripoint nearest_visible_enemy( const tripoint &, const mfaction_id &, int ) const {
        return vision_destination;
    }
    tripoint strongest_scent_track( const tripoint &, int ) const {
        return scent_destination;
    }
    std::vector<tripoint> path_to_nearest_visible_enemy(
        const tripoint &, const mfaction_id &,
        int, enum movement_mode ) const {
        // Unused, just return empty vector.
        return {};
    }

    tripoint nearby_home_area( const tripoint &, int, enum home_area_type ) const {
        return { 0, 0, 0 };
    }

    bool can_move_to( const tripoint &location, int ) const {
        return non_navigable_squares.count( location ) == 0;
    }

private:
    tripoint scent_destination;
    tripoint vision_destination;
    std::set<tripoint> non_navigable_squares;
};

TEST_CASE("monster-group-movement") {

    monster_group test_group;
    monster test_monster;
    test_group.add_monster( test_monster );

    SECTION("Sound based movement.") {
        test_group.apply_stimulus( {0, 0, 0}, { stimulus_sound, {1, 1, 0}, 100, 0 } );
        mock_senses none( {0, 0, 0}, {0, 0, 0} );
        std::map<tripoint, monster_group> moving_groups = test_group.move( {0, 0, 0}, 1, none );
        // Group should have tried to move to 1,1,0
        REQUIRE( moving_groups.count({1, 1, 0}) > 0 );
    }

    SECTION("Sound based movement around an obstacle.") {
        test_group.apply_stimulus( {0, 0, 0}, { stimulus_sound, {2, 2, 0}, 100, 0 } );
        std::set<tripoint> blocked_squares = {{1,1,0}};
        mock_senses none( {0, 0, 0}, {0, 0, 0}, blocked_squares );
        std::map<tripoint, monster_group> moving_groups = test_group.move( {0, 0, 0}, 1, none );
        // Wherever the monster ended up, move it again.
        tripoint location = moving_groups.begin()->first;
        mock_senses none2( location, location, blocked_squares );
        std::map<tripoint, monster_group> moving_groups2 =
            moving_groups.begin()->second.move( location, 1, none2 );
        // And again.
        location = moving_groups2.begin()->first;
        mock_senses none3( location, location, blocked_squares );
        std::map<tripoint, monster_group> moving_groups3 =
            moving_groups2.begin()->second.move( location, 1, none3 );
        // Group should have tried to move to 2,2,0
        REQUIRE( moving_groups3.count({2, 2, 0}) > 0 );
    }

    SECTION("Scent based movement.") {
        mock_senses southeast( { 1, 1, 0 }, { 0, 0, 0 } );
        std::map<tripoint, monster_group> moving_groups = test_group.move( {0, 0, 0}, 1, southeast );
        // Group should have tried to move to 1,1,0
        REQUIRE( moving_groups.count({1, 1, 0}) > 0 );
    }

    SECTION("vision based movement.") {
        mock_senses southeast( { 0, 0, 0}, { 1, 1, 0 } );
        std::map<tripoint, monster_group> moving_groups = test_group.move( {0, 0, 0}, 1, southeast );
        // Group should have tried to move to 1,1,0
        REQUIRE( moving_groups.count({1, 1, 0}) > 0 );
    }
}
