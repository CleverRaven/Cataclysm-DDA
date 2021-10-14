#include "cata_catch.h"
#include "map.h"

#include "action.h"
#include "calendar.h"
#include "game.h"
#include "iexamine.h"
#include "mapdata.h"
#include "map_helpers.h"
#include "point.h"

TEST_CASE( "mapdata_examine" )
{
    map_data_common_t data;
    data.set_examine( iexamine_functions{iexamine::always_true, iexamine::water_source} );

    CHECK( data.has_examine( iexamine::water_source ) );
    CHECK_FALSE( data.has_examine( iexamine::fungus ) );
    CHECK_FALSE( data.has_examine( iexamine::dirtmound ) );
    CHECK_FALSE( data.has_examine( iexamine::none ) );
}

TEST_CASE( "examine_bush" )
{
    clear_map();
    map &m = get_map();
    const tripoint &pine_loc = tripoint_zero;
    const tripoint &elderberry_loc = tripoint_east;

    m.ter_set( pine_loc, ter_id( "t_tree_pine" ) );
    m.ter_set( elderberry_loc, ter_id( "t_tree_elderberry" ) );

    CHECK( m.ter( pine_loc )->has_examine( iexamine::harvest_ter ) );
    CHECK( m.ter( elderberry_loc )->has_examine( iexamine::harvest_ter_nectar ) );

    // In spring, pine is harvestable but elderberry is not
    calendar::turn = calendar::turn_zero;
    CHECK( m.ter( pine_loc )->can_examine( pine_loc ) );
    CHECK_FALSE( m.ter( elderberry_loc )->can_examine( elderberry_loc ) );

    // In summer, both are harvestable
    calendar::turn = calendar::turn_zero + calendar::season_length() + 1_days;
    CHECK( m.ter( pine_loc )->can_examine( pine_loc ) );
    CHECK( m.ter( elderberry_loc )->can_examine( elderberry_loc ) );

    // In fall, just pine again
    calendar::turn = calendar::turn_zero + calendar::season_length() * 2 + 1_days;
    CHECK( m.ter( pine_loc )->can_examine( pine_loc ) );
    CHECK_FALSE( m.ter( elderberry_loc )->can_examine( elderberry_loc ) );
}
