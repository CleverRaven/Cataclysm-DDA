#include <fstream>
#include <memory>
#include <ostream>

#include "catch/catch.hpp"
#include "filesystem.h"
#include "mongroup.h"
#include "npc.h"
#include "overmap.h"

// Intentionally ignoring the name member.
bool operator==( const city &a, const city &b )
{
    return a.pos == b.pos && a.size == b.size;
}
bool operator==( const radio_tower &a, const radio_tower &b )
{
    return a.x == b.x && a.y == b.y && a.strength == b.strength &&
           a.type == b.type && a.message == b.message;
}

void check_test_overmap_data( const overmap &test_map )
{
    // Spot-check a bunch of terrain values.
    // Bottom level, "L 0" in the save
    REQUIRE( test_map.get_ter( 0, 0, -10 ).id() == "empty_rock" );
    REQUIRE( test_map.get_ter( 47, 3, -10 ).id() == "empty_rock" );
    REQUIRE( test_map.get_ter( 48, 3, -10 ).id() == "rock" );
    REQUIRE( test_map.get_ter( 49, 3, -10 ).id() == "rock" );
    REQUIRE( test_map.get_ter( 50, 3, -10 ).id() == "rock" );
    REQUIRE( test_map.get_ter( 51, 3, -10 ).id() == "empty_rock" );
    REQUIRE( test_map.get_ter( 45, 4, -10 ).id() == "empty_rock" );
    REQUIRE( test_map.get_ter( 46, 4, -10 ).id() == "rock" );
    REQUIRE( test_map.get_ter( 47, 4, -10 ).id() == "rock" );
    REQUIRE( test_map.get_ter( 48, 4, -10 ).id() == "slimepit" );
    REQUIRE( test_map.get_ter( 49, 4, -10 ).id() == "slimepit" );
    REQUIRE( test_map.get_ter( 50, 4, -10 ).id() == "slimepit" );
    REQUIRE( test_map.get_ter( 51, 4, -10 ).id() == "rock" );
    REQUIRE( test_map.get_ter( 52, 4, -10 ).id() == "empty_rock" );

    REQUIRE( test_map.get_ter( 179, 179, -10 ).id() == "empty_rock" );
    // Level -9, "L 1" in the save
    REQUIRE( test_map.get_ter( 0, 0, -9 ).id() == "empty_rock" );
    REQUIRE( test_map.get_ter( 44, 1, -9 ).id() == "empty_rock" );
    REQUIRE( test_map.get_ter( 45, 1, -9 ).id() == "rock" );
    REQUIRE( test_map.get_ter( 46, 1, -9 ).id() == "rock" );
    REQUIRE( test_map.get_ter( 47, 1, -9 ).id() == "empty_rock" );
    REQUIRE( test_map.get_ter( 48, 1, -9 ).id() == "rock" );
    REQUIRE( test_map.get_ter( 49, 1, -9 ).id() == "rock" );
    REQUIRE( test_map.get_ter( 50, 1, -9 ).id() == "empty_rock" );

    REQUIRE( test_map.get_ter( 43, 2, -9 ).id() == "empty_rock" );
    REQUIRE( test_map.get_ter( 44, 2, -9 ).id() == "rock" );
    REQUIRE( test_map.get_ter( 45, 2, -9 ).id() == "slimepit" );
    REQUIRE( test_map.get_ter( 46, 2, -9 ).id() == "slimepit" );
    REQUIRE( test_map.get_ter( 47, 2, -9 ).id() == "rock" );
    REQUIRE( test_map.get_ter( 48, 2, -9 ).id() == "slimepit" );
    REQUIRE( test_map.get_ter( 49, 2, -9 ).id() == "slimepit" );
    REQUIRE( test_map.get_ter( 50, 2, -9 ).id() == "rock" );
    REQUIRE( test_map.get_ter( 51, 2, -9 ).id() == "empty_rock" );

    // Level -3, "L 7" in save
    REQUIRE( test_map.get_ter( 0, 0, -3 ).id() == "empty_rock" );
    REQUIRE( test_map.get_ter( 156, 0, -3 ).id() == "empty_rock" );
    REQUIRE( test_map.get_ter( 157, 0, -3 ).id() == "temple_stairs" );
    REQUIRE( test_map.get_ter( 158, 0, -3 ).id() == "empty_rock" );
    REQUIRE( test_map.get_ter( 45, 5, -3 ).id() == "empty_rock" );
    REQUIRE( test_map.get_ter( 46, 5, -3 ).id() == "rock" );
    REQUIRE( test_map.get_ter( 47, 5, -3 ).id() == "rock" );
    REQUIRE( test_map.get_ter( 48, 5, -3 ).id() == "rock" );
    REQUIRE( test_map.get_ter( 49, 5, -3 ).id() == "rock" );
    REQUIRE( test_map.get_ter( 50, 5, -3 ).id() == "empty_rock" );
    REQUIRE( test_map.get_ter( 133, 5, -3 ).id() == "empty_rock" );
    REQUIRE( test_map.get_ter( 134, 5, -3 ).id() == "mine" );
    REQUIRE( test_map.get_ter( 135, 5, -3 ).id() == "empty_rock" );

    // Ground level
    REQUIRE( test_map.get_ter( 0, 0, 0 ).id() == "field" );
    REQUIRE( test_map.get_ter( 23, 0, 0 ).id() == "field" );
    REQUIRE( test_map.get_ter( 24, 0, 0 ).id() == "forest_thick" );
    REQUIRE( test_map.get_ter( 25, 0, 0 ).id() == "forest_thick" );
    REQUIRE( test_map.get_ter( 26, 0, 0 ).id() == "forest_thick" );
    REQUIRE( test_map.get_ter( 27, 0, 0 ).id() == "forest" );
    REQUIRE( test_map.get_ter( 28, 0, 0 ).id() == "forest" );
    REQUIRE( test_map.get_ter( 29, 0, 0 ).id() == "forest" );
    REQUIRE( test_map.get_ter( 30, 0, 0 ).id() == "forest" );

    // Sky
    REQUIRE( test_map.get_ter( 0, 0, 1 ).id() == "open_air" );
    REQUIRE( test_map.get_ter( 179, 179, 1 ).id() == "open_air" );

    REQUIRE( test_map.get_ter( 0, 0, 2 ).id() == "open_air" );
    REQUIRE( test_map.get_ter( 179, 179, 2 ).id() == "open_air" );

    REQUIRE( test_map.get_ter( 0, 0, 10 ).id() == "open_air" );
    REQUIRE( test_map.get_ter( 179, 179, 10 ).id() == "open_air" );

    // Spot-check a few of the monster groups.
    std::vector<mongroup> expected_groups{
        {"GROUP_ANT", {0, 0, -1}, 1, 3, {0, 0, 0}, 0, false, false, false},
        {"GROUP_TRIFFID", {0, 132, 0}, 1, 1, {0, 0, 0}, 0, false, false, false},
        {"GROUP_ANT", {0, 189, 0}, 1, 1, {0, 0, 0}, 0, false, false, false},
        {"GROUP_FUNGI", {0, 288, 0}, 1, 1, {0, 0, 0}, 0, false, false, false},
        {"GROUP_ANT", {1, 0, -1}, 1, 3, {0, 0, 0}, 0, false, false, false},
        {"GROUP_ANT", {1, 0, 0}, 1, 2, {0, 0, 0}, 0, false, false, false},
        {"GROUP_TRIFFID", {1, 137, 0}, 1, 2, {0, 0, 0}, 0, false, false, false},
        {"GROUP_WORM", {2, 67, 0}, 1, 1, {0, 0, 0}, 0, false, false, false},
        {"GROUP_FUNGI_FLOWERS", {2, 150, 0}, 1, 1, {0, 0, 0}, 0, false, false, false},
        {"GROUP_FUNGI_FLOWERS", {5, 150, 0}, 1, 1, {0, 0, 0}, 0, false, false, false},
        {"GROUP_ANT", {6, 365, -1}, 1, 1, {0, 0, 0}, 0, false, false, false},
        {"GROUP_ANT", {1, 8, -2}, 1, 2, {100, 50, 0}, 0, true, false, false},
        {"GROUP_GOO", {2, 9, -1}, 1, 4, {25, 75, 0}, 10, false, true, false},
        {"GROUP_BEE", {3, 10, 0}, 1, 6, {92, 64, 0}, 20, false, false, true},
        {"GROUP_CHUD", {4, 11, -2}, 1, 8, {88, 55, 0}, 30, true, true, false},
        {"GROUP_SPIRAL", {5, 12, -1}, 1, 10, {62, 47, 0}, 40, false, true, true},
        {"GROUP_RIVER", {6, 13, 0}, 1, 12, {94, 72, 0}, 50, true, false, true},
        {"GROUP_SWAMP", {7, 14, -2}, 1, 14, {37, 85, 0}, 60, true, true, true}
    };

    for( const mongroup &group : expected_groups ) {
        REQUIRE( test_map.mongroup_check( group ) );
    }

    // Only a few cities, so check them all.
    std::vector<city> expected_cities {{145, 53, 9}, {24, 60, 7}, {90, 114, 2}, {108, 129, 9}, {83, 26, 10},
        {140, 89, 2}, {71, 33, 2}, {67, 111, 2}, {97, 144, 9}, {96, 166, 2}};
    REQUIRE( test_map.cities.size() == expected_cities.size() );
    for( const auto &candidate_city : test_map.cities ) {
        REQUIRE( std::find( expected_cities.begin(), expected_cities.end(),
                            candidate_city ) != expected_cities.end() );
    }
    // Check all the roads too.
    // Roads are getting size set to 0, but I expect -1.
    std::vector<city> expected_roads = {{179, 126, -1}, {136, 179, -1}};
    REQUIRE( test_map.roads_out.size() == expected_roads.size() );
    for( const auto &candidate_road : test_map.roads_out ) {
        REQUIRE( std::find( expected_roads.begin(), expected_roads.end(),
                            candidate_road ) != expected_roads.end() );
    }
    // Check the radio towers.
    std::vector<radio_tower> expected_towers{
        {2, 42, 122, "This is FEMA camp 121.  Supplies are limited, please bring supplemental food, water, and bedding.  This is FEMA camp 121.  A designated long-term emergency shelter.", MESSAGE_BROADCAST},
        {36, 300, 193, "This is FEMA camp 18150.  Supplies are limited, please bring supplemental food, water, and bedding.  This is FEMA camp 18150.  A designated long-term emergency shelter.", MESSAGE_BROADCAST},
        {56, 194, 92, "This is automated emergency shelter beacon 2897.  Supplies, amenities and shelter are stocked.", MESSAGE_BROADCAST},
        {62, 208, 176, "This is FEMA camp 31104.  Supplies are limited, please bring supplemental food, water, and bedding.  This is FEMA camp 31104.  A designated long-term emergency shelter.", MESSAGE_BROADCAST},
        {64, 42, 190, "", WEATHER_RADIO}, {92, 146, 100, "", WEATHER_RADIO},
        {126, 194, 112, "This is FEMA camp 6397.  Supplies are limited, please bring supplemental food, water, and bedding.  This is FEMA camp 6397.  A designated long-term emergency shelter.", MESSAGE_BROADCAST},
        {142, 128, 114, "This is FEMA camp 7164.  Supplies are limited, please bring supplemental food, water, and bedding.  This is FEMA camp 7164.  A designated long-term emergency shelter.", MESSAGE_BROADCAST},
        {236, 168, 115, "", WEATHER_RADIO},
        {240, 352, 95, "This is automated emergency shelter beacon 120176.  Supplies, amenities and shelter are stocked.", MESSAGE_BROADCAST},
        {244, 162, 150, "This is emergency broadcast station 12281.  Please proceed quickly and calmly to your designated evacuation point.", MESSAGE_BROADCAST},
        {282, 48, 190, "This is emergency broadcast station 14124.  Please proceed quickly and calmly to your designated evacuation point.", MESSAGE_BROADCAST},
        {306, 66, 90, "This is emergency broadcast station 15333.  Please proceed quickly and calmly to your designated evacuation point.", MESSAGE_BROADCAST}};
    REQUIRE( test_map.radios.size() == expected_towers.size() );
    for( const auto &candidate_tower : test_map.radios ) {
        REQUIRE( std::find( expected_towers.begin(), expected_towers.end(),
                            candidate_tower ) != expected_towers.end() );
    }
    // Spot-check some monsters.
    std::vector<std::pair<tripoint, monster>> expected_monsters{
        {{251, 86, 0}, { mtype_id( "mon_zombie" ), {140, 23, 0}}},
        {{253, 87, 0}, { mtype_id( "mon_zombie" ), {136, 25, 0}}},
        {{259, 95, 0}, { mtype_id( "mon_zombie" ), {143, 122, 0}}},
        {{259, 94, 0}, { mtype_id( "mon_zombie" ), {139, 109, 0}}},
        {{259, 91, 0}, { mtype_id( "mon_dog" ), {139, 82, 0}}},
        {{194, 87, -3}, { mtype_id( "mon_irradiated_wanderer_4" ), {119, 73, -3}}},
        {{194, 87, -3}, { mtype_id( "mon_charred_nightmare" ), {117, 83, -3}}},
        {{142, 96, 0}, { mtype_id( "mon_deer" ), {16, 109, 0}}},
        {{196, 66, -1}, { mtype_id( "mon_turret" ), {17, 65, -1}}},
        {{196, 63, -1}, { mtype_id( "mon_broken_cyborg" ), {19, 26, -1}}}
    };
    for( const auto &candidate_monster : expected_monsters ) {
        REQUIRE( test_map.monster_check( candidate_monster ) );
    }
    // Check NPCs.  They're complicated enough that I'm just going to spot-check some stats.
    for( const std::shared_ptr<npc> &test_npc : test_map.get_npcs() ) {
        if( test_npc->disp_name() == "Felix Brandon" ) {
            REQUIRE( test_npc->get_str() == 7 );
            REQUIRE( test_npc->get_dex() == 8 );
            REQUIRE( test_npc->get_int() == 7 );
            REQUIRE( test_npc->get_per() == 10 );
            REQUIRE( test_npc->get_skill_level( skill_id( "barter" ) ) == 4 );
            REQUIRE( test_npc->get_skill_level( skill_id( "driving" ) ) == 2 );
            REQUIRE( test_npc->get_skill_level( skill_id( "firstaid" ) ) == 7 );
            REQUIRE( test_npc->get_skill_level( skill_id( "mechanics" ) ) == 5 );
            REQUIRE( test_npc->get_skill_level( skill_id( "dodge" ) ) == 3 );
            REQUIRE( test_npc->get_skill_level( skill_id( "launcher" ) ) == 3 );
            REQUIRE( test_npc->pos() == tripoint( 168, 66, 0 ) );
        } else if( test_npc->disp_name() == "Mariann Araujo" ) {
            REQUIRE( test_npc->get_str() == 11 );
            REQUIRE( test_npc->get_dex() == 9 );
            REQUIRE( test_npc->get_int() == 10 );
            REQUIRE( test_npc->get_per() == 10 );
            REQUIRE( test_npc->get_skill_level( skill_id( "barter" ) ) == 4 );
            REQUIRE( test_npc->get_skill_level( skill_id( "driving" ) ) == 0 );
            REQUIRE( test_npc->get_skill_level( skill_id( "firstaid" ) ) == 5 );
            REQUIRE( test_npc->get_skill_level( skill_id( "bashing" ) ) == 5 );
            REQUIRE( test_npc->get_skill_level( skill_id( "dodge" ) ) == 4 );
            REQUIRE( test_npc->pos() == tripoint( 72, 54, 0 ) );
        } else {
            // Unrecognized NPC, fail.
            REQUIRE( false );
        }
    }
}

TEST_CASE( "Reading a legacy overmap save." )
{

    std::string legacy_save_name = "tests/data/legacy_0.C_overmap.sav";
    std::string new_save_name = "tests/data/jsionized_overmap.sav";
    std::unique_ptr<overmap> test_map = std::unique_ptr<overmap>( new overmap( 0, 0 ) );
    std::ifstream fin;

    fin.open( legacy_save_name.c_str(), std::ifstream::binary );
    REQUIRE( fin.is_open() );
    test_map->unserialize( fin );
    fin.close();
    check_test_overmap_data( *test_map );

    std::ofstream fout;

    fout.open( new_save_name.c_str(), std::ofstream::binary );
    REQUIRE( fout.is_open() );
    test_map->serialize( fout );
    fout.close();

    std::unique_ptr<overmap> test_map_2 = std::unique_ptr<overmap>( new overmap( 0, 0 ) );

    fin.open( new_save_name.c_str(), std::ifstream::binary );
    REQUIRE( fin.is_open() );
    test_map_2->unserialize( fin );
    fin.close();

    check_test_overmap_data( *test_map_2 );

    // Now clean up.
    remove_file( new_save_name );
}
