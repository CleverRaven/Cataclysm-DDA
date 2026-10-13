#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "basecamp.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "cata_utility.h"
#include "cata_variant.h"
#include "character.h"
#include "clzones.h"
#include "coordinates.h"
#include "faction.h"
#include "item.h"
#include "item_components.h"
#include "itype.h"
#include "map.h"
#include "map_helpers.h"
#include "map_scale_constants.h"
#include "mapgendata.h"
#include "mission_companion.h"
#include "overmapbuffer.h"
#include "player_helpers.h"
#include "point.h"
#include "stomach.h"
#include "type_id.h"
#include "value_ptr.h"

static const itype_id itype_hacksaw( "hacksaw" );
static const itype_id itype_metal_tank( "metal_tank" );
static const itype_id itype_pipe( "pipe" );
static const itype_id itype_test_100_kcal( "test_100_kcal" );
static const itype_id itype_test_200_kcal( "test_200_kcal" );
static const itype_id itype_test_500_kcal( "test_500_kcal" );

static const recipe_id recipe_fbmh_2_room_1_1( "fbmh_2_room_1_1" );
static const recipe_id recipe_test_base_stove_1( "test_base_stove_1" );

static const vitamin_id vitamin_mutagen( "mutagen" );
static const vitamin_id vitamin_mutant_toxin( "mutant_toxin" );

static const zone_type_id zone_type_CAMP_FOOD( "CAMP_FOOD" );
static const zone_type_id zone_type_CAMP_STORAGE( "CAMP_STORAGE" );

TEST_CASE( "camp_calorie_counting", "[camp]" )
{
    clear_avatar();
    clear_map_without_vision();
    map &m = get_map();
    const tripoint_abs_ms zone_loc = m.get_abs( tripoint_bub_ms{ 5, 5, 0 } );
    REQUIRE( m.inbounds( zone_loc ) );
    mapgen_place_zone( zone_loc, zone_loc, zone_type_CAMP_FOOD, your_fac, {},
                       "food" );
    mapgen_place_zone( zone_loc, zone_loc, zone_type_CAMP_STORAGE, your_fac, {},
                       "storage" );
    faction *camp_faction = get_player_character().get_faction();
    const tripoint_abs_omt this_omt = project_to<coords::omt>( zone_loc );
    m.add_camp( this_omt, "faction_camp" );
    std::optional<basecamp *> bcp = overmap_buffer.find_camp( this_omt.xy() );
    REQUIRE( !!bcp );
    basecamp *test_camp = *bcp;
    test_camp->set_owner( your_fac );
    WHEN( "a base item is added to larder" ) {
        camp_faction->empty_food_supply();
        item test_100_kcal( itype_test_100_kcal );
        tripoint_bub_ms zone_local = m.get_bub( zone_loc );
        m.i_clear( zone_local );
        m.add_item_or_charges( zone_local, test_100_kcal );
        REQUIRE( m.has_items( zone_local ) );
        test_camp->distribute_food();
        CHECK( camp_faction->food_supply().kcal() == 100 );
    }

    WHEN( "an item with inherited components is added to larder" ) {
        camp_faction->empty_food_supply();
        item test_100_kcal( itype_test_100_kcal );
        item test_200_kcal( itype_test_200_kcal );
        item_components made_of;
        made_of.add( test_100_kcal );
        made_of.add( test_100_kcal );
        // Setting the actual components. This will return 185 unless it's actually made up of two 100kcal components!
        test_200_kcal.components = made_of;
        tripoint_bub_ms zone_local = m.get_bub( zone_loc );
        m.i_clear( zone_local );
        m.add_item_or_charges( zone_local, test_200_kcal );
        test_camp->distribute_food();
        CHECK( camp_faction->food_supply().kcal() == 200 );
    }

    WHEN( "an item with vitamins is added to larder" ) {
        camp_faction->empty_food_supply();
        item test_500_kcal( itype_test_500_kcal );
        tripoint_bub_ms zone_local = m.get_bub( zone_loc );
        m.i_clear( zone_local );
        m.add_item_or_charges( zone_local, test_500_kcal );
        test_camp->distribute_food();
        REQUIRE( camp_faction->food_supply().kcal() == 500 );
        REQUIRE( camp_faction->food_supply().get_vitamin( vitamin_mutant_toxin ) == 100 );
        REQUIRE( camp_faction->food_supply().get_vitamin( vitamin_mutagen ) == 200 );
    }

    WHEN( "a larder with stored calories and vitamins has food withdrawn" ) {
        camp_faction->empty_food_supply();
        std::map<time_point, nutrients> added_food;
        added_food[calendar::turn_zero].calories = 100000;
        added_food[calendar::turn_zero].set_vitamin( vitamin_mutant_toxin, 100 );
        added_food[calendar::turn_zero].set_vitamin( vitamin_mutagen, 200 );
        camp_faction->add_to_food_supply( added_food );
        REQUIRE( camp_faction->food_supply().kcal() == 100 );
        REQUIRE( camp_faction->food_supply().get_vitamin( vitamin_mutant_toxin ) == 100 );
        REQUIRE( camp_faction->food_supply().get_vitamin( vitamin_mutagen ) == 200 );
        // Now withdraw 15% of the total calories, this should also draw out 15% of the stored vitamins.
        test_camp->camp_food_supply( -15 );
        CHECK( camp_faction->food_supply().kcal() == 85 );
        CHECK( camp_faction->food_supply().get_vitamin( vitamin_mutant_toxin ) == 85 );
        CHECK( camp_faction->food_supply().get_vitamin( vitamin_mutagen ) == 170 );
    }

    WHEN( "a larder with perishable food passes the expiry date" ) {
        restore_on_out_of_scope restore_calendar_turn( calendar::turn );
        camp_faction->empty_food_supply();
        // non-perishable food
        std::map<time_point, nutrients> added_food;
        added_food[calendar::turn_zero].calories = 100000;
        added_food[calendar::turn_zero].set_vitamin( vitamin_mutant_toxin, 100 );
        added_food[calendar::turn_zero].set_vitamin( vitamin_mutagen, 200 );

        camp_faction->add_to_food_supply( added_food );
        REQUIRE( camp_faction->food_supply().kcal() == 100 );
        REQUIRE( camp_faction->food_supply().get_vitamin( vitamin_mutant_toxin ) == 100 );
        REQUIRE( camp_faction->food_supply().get_vitamin( vitamin_mutagen ) == 200 );

        // remove non-perishable from added
        added_food.erase( added_food.begin() );
        // perishable food
        added_food[calendar::turn + 7_days].calories = 150000;
        added_food[calendar::turn + 7_days].set_vitamin( vitamin_mutant_toxin, 200 );
        added_food[calendar::turn + 7_days].set_vitamin( vitamin_mutagen, 100 );

        camp_faction->add_to_food_supply( added_food );
        REQUIRE( camp_faction->food_supply().kcal() == 250 );
        REQUIRE( camp_faction->food_supply().get_vitamin( vitamin_mutant_toxin ) == 300 );
        REQUIRE( camp_faction->food_supply().get_vitamin( vitamin_mutagen ) == 300 );

        // advance time
        calendar::turn += 14_days;

        CHECK( camp_faction->food_supply().kcal() == 100 );
        CHECK( camp_faction->food_supply().get_vitamin( vitamin_mutant_toxin ) == 100 );
        CHECK( camp_faction->food_supply().get_vitamin( vitamin_mutagen ) == 200 );
    }

    WHEN( "an item that expires is added to larder" ) {
        restore_on_out_of_scope restore_calendar_turn( calendar::turn );
        camp_faction->empty_food_supply();
        item test_100_kcal( itype_test_100_kcal );
        tripoint_bub_ms zone_local = m.get_bub( zone_loc );
        m.i_clear( zone_local );
        m.add_item_or_charges( zone_local, test_100_kcal );
        test_camp->distribute_food();
        REQUIRE( camp_faction->food_supply().kcal() == 100 );
        REQUIRE( test_100_kcal.type->comestible != nullptr );
        REQUIRE( test_100_kcal.type->comestible->spoils == 360_days );

        calendar::turn += 365_days;

        CHECK( camp_faction->food_supply().kcal() == 0 );
    }
    overmap_buffer.clear_camps( this_omt.xy() );
}

// player-owned camp of the given type at mid-map; caller removes it with clear_camps
static basecamp &place_test_camp( std::string_view camp_type, tripoint_abs_omt &camp_omt )
{
    map &here = get_map();
    const tripoint_bub_ms mid{ MAPSIZE_X / 2, MAPSIZE_Y / 2, 0 };
    camp_omt = project_to<coords::omt>( here.get_abs( mid ) );
    here.add_camp( camp_omt, "faction_camp" );
    std::optional<basecamp *> bcp = overmap_buffer.find_camp( camp_omt.xy() );
    REQUIRE( bcp );
    basecamp &camp = **bcp;
    camp.set_owner( your_fac );
    camp.define_camp( camp_omt, camp_type, false );
    return camp;
}

static std::vector<mission_entry> offered_upgrades( const mission_data &missions )
{
    std::vector<mission_entry> ret;
    for( const std::vector<mission_entry> &tab : missions.entries ) {
        for( const mission_entry &e : tab ) {
            if( !e.id.ret && e.id.id.id == Camp_Upgrade && !e.id.id.parameters.empty() ) {
                ret.push_back( e );
            }
        }
    }
    return ret;
}

TEST_CASE( "camp_upgrade_missions_show_blueprint_and_parameter_names", "[camp]" )
{
    clear_avatar();
    clear_map_without_vision();
    map &here = get_map();
    tripoint_abs_omt camp_omt;

    GIVEN( "modular field hub whose next room takes a wall material" ) {
        basecamp &camp = place_test_camp( "faction_base_modular_hub_field_version_2_0", camp_omt );
        on_out_of_scope remove_camp( [&camp_omt]() {
            overmap_buffer.clear_camps( camp_omt.xy() );
        } );
        WHEN( "base direction's missions are listed" ) {
            camp.form_crafting_inventory( here );
            mission_data missions;
            camp.get_available_missions_by_dir( missions, base_camps::base_dir );
            const std::vector<mission_entry> upgrades = offered_upgrades( missions );
            REQUIRE( std::any_of( upgrades.begin(), upgrades.end(), []( const mission_entry & e ) {
                return !e.id.id.mapgen_args.map.empty();
            } ) );

            THEN( "each upgrade displays the same name as name_display_of" ) {
                for( const mission_entry &e : upgrades ) {
                    CAPTURE( e.id.id.parameters );
                    CHECK( e.name_display == camp.name_display_of( e.id.id ) );
                }
            }
            THEN( "wooden room shows its blueprint and wall material" ) {
                const auto wooden_room = std::find_if( upgrades.begin(), upgrades.end(),
                []( const mission_entry & e ) {
                    const auto palette = e.id.id.mapgen_args.map.find( "fbmh_2_construction_palette" );
                    return e.id.id.parameters == recipe_fbmh_2_room_1_1.str() &&
                           palette != e.id.id.mapgen_args.map.end() &&
                           palette->second.get_string() == "fbmh_2_wood_palette";
                } );
                REQUIRE( wooden_room != upgrades.end() );
                CHECK( wooden_room->name_display ==
                       "[B] Upgrade Camp northeast shack (Wooden walls and wooden roof)" );
            }
        }
    }

    GIVEN( "camp that doesn't offer the room" ) {
        basecamp &camp = place_test_camp( "faction_base_bare_bones_NPC_camp_0", camp_omt );
        on_out_of_scope remove_camp( [&camp_omt]() {
            overmap_buffer.clear_camps( camp_omt.xy() );
        } );
        THEN( "room mission is named no longer valid" ) {
            const mission_id room{ Camp_Upgrade, recipe_fbmh_2_room_1_1.str(), {}, base_camps::base_dir };
            const std::string name = camp.name_display_of( room );
            CAPTURE( name );
            CHECK( string_ends_with( name, "<No longer valid construction>" ) );
        }
    }
}
TEST_CASE( "camp_upgrade_offers_follow_the_camp_storage", "[camp]" )
{
    clear_avatar();
    clear_map_without_vision();
    map &here = get_map();
    tripoint_abs_omt camp_omt;
    basecamp &camp = place_test_camp( "faction_base_bare_bones_NPC_camp_0", camp_omt );
    faction *camp_faction = get_player_character().get_faction();
    on_out_of_scope cleanup( [&camp_omt, camp_faction]() {
        overmap_buffer.clear_camps( camp_omt.xy() );
        camp_faction->empty_food_supply();
    } );
    camp_faction->empty_food_supply();
    std::map<time_point, nutrients> food;
    food[calendar::turn_zero].calories = 10000 * 1000;
    camp_faction->add_to_food_supply( food );

    const tripoint_bub_ms storage{ MAPSIZE_X / 2 + 1, MAPSIZE_Y / 2, 0 };
    camp.set_storage_tiles( { here.get_abs( storage ) } );
    here.i_clear( storage );

    const auto stove_offer = [&]() {
        camp.form_crafting_inventory( here );
        mission_data missions;
        camp.get_available_missions_by_dir( missions, base_camps::base_dir );
        const std::vector<mission_entry> upgrades = offered_upgrades( missions );
        const auto stove = std::find_if( upgrades.begin(), upgrades.end(),
        []( const mission_entry & e ) {
            return e.id.id.parameters == recipe_test_base_stove_1.str();
        } );
        REQUIRE( stove != upgrades.end() );
        CAPTURE( stove->name_display );
        return stove->possible;
    };

    WHEN( "storage: hacksaw, metal tank, pipe" ) {
        here.add_item_or_charges( storage, item( itype_hacksaw ) );
        here.add_item_or_charges( storage, item( itype_metal_tank ) );
        here.add_item_or_charges( storage, item( itype_pipe ) );
        THEN( "stove can be built" ) {
            CHECK( stove_offer() );
        }
    }
    WHEN( "storage: tank + pipe, no saw" ) {
        here.add_item_or_charges( storage, item( itype_metal_tank ) );
        here.add_item_or_charges( storage, item( itype_pipe ) );
        THEN( "stove can't be built" ) {
            CHECK_FALSE( stove_offer() );
        }
    }
    WHEN( "storage only has the hacksaw" ) {
        here.add_item_or_charges( storage, item( itype_hacksaw ) );
        THEN( "stove can't be built" ) {
            CHECK_FALSE( stove_offer() );
        }
    }
}
// TODO: Tests for: Check calorie display at various activity levels, camp crafting works as expected (consumes inputs, returns outputs+byproducts, costs calories)
