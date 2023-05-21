#include <vector>

#include "cata_catch.h"
#include "game.h"
#include "item.h"
#include "item_group.h"
#include "map_helpers.h"
#include "mutation.h"
#include "profession.h"
#include "recipe.h"
#include "test_data.h"
#include "vehicle.h"
#include "veh_type.h"

static std::string get_section_name( const spawn_type &type )
{
    switch( type ) {
        case spawn_type::item_group:
            return "items spawned by itemgroups";
        case spawn_type::recipe:
            return "items spawned by recipe";
        case spawn_type::vehicle:
            return "items spawned by vehicle";
        case spawn_type::profession:
            return "items spawned by profession substitution";
        case spawn_type::map:
            return "items spawned by map::spawn_item";
        default:
            return "unknown type";
    }

    return "unknown type";
}

TEST_CASE( "correct amounts of an item spawn inside a container", "[item_spawn]" )
{
    REQUIRE_FALSE( test_data::container_spawn_data.empty() );

    for( std::pair<const spawn_type, std::vector<container_spawn_test_data>> &spawn_category :
         test_data::container_spawn_data ) {
        SECTION( get_section_name( spawn_category.first ) ) {
            REQUIRE_FALSE( spawn_category.second.empty() );
            for( const container_spawn_test_data &cs_data : spawn_category.second ) {
                GIVEN( cs_data.given ) {
                    std::vector<item> items;

                    switch( spawn_category.first ) {
                        case spawn_type::item_group:
                            items = item_group::items_from( cs_data.group );
                            break;
                        case spawn_type::recipe:
                            items = cs_data.recipe->create_results();
                            break;
                        case spawn_type::vehicle: {
                            clear_map();
                            map &here = get_map();
                            here.add_vehicle( cs_data.vehicle, point_zero, 0_degrees, -1, 0 );
                            REQUIRE( here.get_vehicles().size() == 1 );
                            vehicle *veh = here.get_vehicles()[0].v;
                            int part = veh->avail_part_with_feature( point_zero, "CARGO" );
                            REQUIRE( part >= 0 );
                            for( item &it : veh->get_items( part ) ) {
                                items.push_back( it );
                            }
                            break;
                        }
                        case spawn_type::profession: {
                            std::vector<trait_id> traits;
                            for( const trait_and_var &trait : cs_data.profession->get_locked_traits() ) {
                                traits.push_back( trait.trait );
                            }
                            std::list<item> prof_items = cs_data.profession->items( false, traits );
                            items.insert( items.end(), prof_items.begin(), prof_items.end() );
                            break;
                        }
                        case spawn_type::map: {
                            clear_map();
                            map &here = get_map();
                            here.spawn_item( tripoint_zero, cs_data.item, 1, cs_data.charges );
                            for( item &it : here.i_at( tripoint_zero ) ) {
                                items.push_back( it );
                            }
                            break;
                        }
                        default:
                            continue;
                    }

                    REQUIRE( items.size() == 1 );
                    CAPTURE( items[0].tname() );
                    REQUIRE_FALSE( items[0].empty() );

                    int count = 0;
                    for( const item *it : items[0].all_items_top() ) {
                        count += it->count();
                    }

                    CHECK( count == cs_data.expected_amount );
                }
            }
        }
    }
}
