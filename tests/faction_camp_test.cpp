#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
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
#include "debug.h"
#include "faction.h"
#include "game_constants.h"
#include "item.h"
#include "item_components.h"
#include "itype.h"
#include "list.h" // IWYU pragma: keep
#include "map.h"
#include "map_helpers.h"
#include "map_scale_constants.h"
#include "mapgendata.h"
#include "mission_companion.h"
#include "npc.h"
#include "options_helpers.h"
#include "overmapbuffer.h"
#include "player_helpers.h"
#include "point.h"
#include "stomach.h"
#include "type_id.h"
#include "value_ptr.h"

static const efftype_id effect_nausea( "nausea" );

static const faction_id faction_your_followers( "your_followers" );

static const itype_id itype_hacksaw( "hacksaw" );
static const itype_id itype_metal_tank( "metal_tank" );
static const itype_id itype_pipe( "pipe" );
static const itype_id itype_test_100_kcal( "test_100_kcal" );
static const itype_id itype_test_200_kcal( "test_200_kcal" );
static const itype_id itype_test_500_kcal( "test_500_kcal" );

static const recipe_id recipe_fbmh_2_room_1_1( "fbmh_2_room_1_1" );
static const recipe_id recipe_test_base_stove_1( "test_base_stove_1" );

static const skill_id skill_menial( "menial" );

static const trait_id trait_SAPROPHAGE( "SAPROPHAGE" );

static const vitamin_id vitamin_human_flesh_vitamin( "human_flesh_vitamin" );
static const vitamin_id vitamin_iron( "iron" );
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

// larder of kcal with iron in proportion, 1 unit per 10 kcal
static void stock_larder( faction &fac, const int kcal )
{
    fac.empty_food_supply();
    nutrients stock;
    stock.calories = kcal * 1000;
    stock.set_vitamin( vitamin_iron, kcal / 10 );
    fac.debug_food_supply().emplace_back( calendar::turn_zero, stock );
    REQUIRE( fac.food_supply().kcal() == kcal );
    REQUIRE( fac.food_supply().get_vitamin( vitamin_iron ) == kcal / 10 );
}

// 1000 kcal that keep, 200 kcal that expire, iron in proportion
static void stock_larder_with_expiry( faction &fac, const time_point &expiry )
{
    fac.empty_food_supply();
    nutrients keeps;
    keeps.calories = 1000 * 1000;
    keeps.set_vitamin( vitamin_iron, 100 );
    nutrients expires;
    expires.calories = 200 * 1000;
    expires.set_vitamin( vitamin_iron, 20 );
    fac.debug_food_supply().emplace_back( calendar::turn_zero, keeps );
    fac.debug_food_supply().emplace_back( expiry, expires );
    REQUIRE( fac.food_supply().kcal() == 1200 );
}

static nutrients stock_expiring_at( faction &fac, const time_point &expiry )
{
    for( const std::pair<time_point, nutrients> &entry : fac.debug_food_supply() ) {
        if( entry.first == expiry ) {
            return entry.second;
        }
    }
    return nutrients();
}

TEST_CASE( "camp_meals_give_the_calories_they_take", "[camp]" )
{
    clear_avatar();
    clear_map_without_vision();
    get_player_character().camps.clear();
    tripoint_abs_omt camp_omt;
    basecamp &camp = place_test_camp( "faction_base_bare_bones_NPC_camp_0", camp_omt );
    faction &fac = *get_player_character().get_faction();
    const tripoint_bub_ms mid{ MAPSIZE_X / 2, MAPSIZE_Y / 2, 0 };
    npc &guy = spawn_npc( mid.xy(), "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.stomach.empty();
    guy.guts.empty();
    guy.set_stored_kcal( guy.get_healthy_kcal() / 2 );
    guy.set_hunger( 0 );

    GIVEN( "larder of 1000 kcal with iron" ) {
        stock_larder( fac, 1000 );
        WHEN( "worker is fed 300 kcal" ) {
            const camp_food_portion portion = camp.camp_food_supply( -300 );
            THEN( "portion is what was taken and nourishes" ) {
                CHECK( portion.total().kcal() == 300 );
                CHECK( portion.total().get_vitamin( vitamin_iron ) == 30 );
                CHECK( camp.feed_workers( guy, portion, camp_meal::personal ) == 300 );
                CHECK( guy.stomach.get_calories() == 300 );
                CHECK( fac.food_supply().kcal() == 700 );
                CHECK( fac.food_supply().get_vitamin( vitamin_iron ) == 70 );
            }
        }
        WHEN( "nauseous worker is fed" ) {
            guy.add_effect( effect_nausea, 10_minutes );
            const camp_food_portion portion = camp.camp_food_supply( -300 );
            REQUIRE( fac.food_supply().kcal() == 700 );
            THEN( "nothing is eaten, larder is as it was" ) {
                CHECK( camp.feed_workers( guy, portion, camp_meal::personal ) == 0 );
                CHECK( guy.stomach.get_calories() == 0 );
                CHECK( fac.food_supply().calories == 1000 * 1000 );
                CHECK( fac.food_supply().get_vitamin( vitamin_iron ) == 100 );
            }
        }
        WHEN( "worker who won't eat human flesh is fed from a larder that holds some" ) {
            nutrients flesh;
            flesh.set_vitamin( vitamin_human_flesh_vitamin, 10 );
            fac.debug_food_supply().emplace_back( calendar::turn_zero, flesh );
            const camp_food_portion portion = camp.camp_food_supply( -300 );
            REQUIRE( portion.total().get_vitamin( vitamin_human_flesh_vitamin ) > 0 );
            THEN( "nothing is eaten, larder is as it was" ) {
                CHECK( camp.feed_workers( guy, portion, camp_meal::personal ) == 0 );
                CHECK( guy.stomach.get_calories() == 0 );
                CHECK( fac.food_supply().calories == 1000 * 1000 );
                CHECK( fac.food_supply().get_vitamin( vitamin_iron ) == 100 );
                CHECK( fac.food_supply().get_vitamin( vitamin_human_flesh_vitamin ) == 10 );
            }
        }
        WHEN( "three workers share a portion that won't divide evenly" ) {
            std::vector<std::reference_wrapper<Character>> party{ guy };
            for( int i = 1; i <= 2; ++i ) {
                npc &other = spawn_npc( point_bub_ms( mid.x(), mid.y() + i ), "test_talker" );
                clear_character( other, true );
                other.set_fac( faction_your_followers );
                other.stomach.empty();
                other.guts.empty();
                other.set_stored_kcal( other.get_healthy_kcal() / 2 );
                other.set_hunger( 0 );
                party.emplace_back( other );
            }
            const camp_food_portion portion = camp.camp_food_supply( -310 );
            REQUIRE( portion.total().calories == 310 * 1000 );
            REQUIRE( portion.total().get_vitamin( vitamin_iron ) == 31 );
            const auto stomach_kcal = [&party]( const int i ) {
                return party[i].get().stomach.get_calories();
            };
            THEN( "all of it is eaten, no more, no less" ) {
                CHECK( camp.feed_workers( party, portion, camp_meal::personal ) == 310 );
                // shares of what's left, in whole kcal
                CHECK( stomach_kcal( 0 ) == 104 );
                CHECK( stomach_kcal( 1 ) == 103 );
                CHECK( stomach_kcal( 2 ) == 103 );
                CHECK( fac.food_supply().calories == 690 * 1000 );
                CHECK( fac.food_supply().get_vitamin( vitamin_iron ) == 69 );
            }
            THEN( "share of one who can't eat goes back exactly" ) {
                party[1].get().add_effect( effect_nausea, 10_minutes );
                // shares: 104, 103, 103 kcal; 11, 10, 10 iron
                CHECK( camp.feed_workers( party, portion, camp_meal::personal ) == 207 );
                CHECK( stomach_kcal( 0 ) == 104 );
                CHECK( stomach_kcal( 1 ) == 0 );
                CHECK( stomach_kcal( 2 ) == 103 );
                CHECK( fac.food_supply().calories == 690 * 1000 + 103 * 1000 );
                CHECK( fac.food_supply().get_vitamin( vitamin_iron ) == 79 );
            }
        }
        WHEN( "worker whose diet refuses fresh food is fed" ) {
            guy.set_mutation( trait_SAPROPHAGE );
            const camp_food_portion portion = camp.camp_food_supply( -300 );
            REQUIRE( fac.food_supply().kcal() == 700 );
            THEN( "nothing eaten, portion returned" ) {
                CHECK( camp.feed_workers( guy, portion, camp_meal::personal ) == 0 );
                CHECK( guy.stomach.get_calories() == 0 );
                CHECK( fac.food_supply().kcal() == 1000 );
                CHECK( fac.food_supply().get_vitamin( vitamin_iron ) == 100 );
            }
        }
        WHEN( "worker returns from a mission paid for when it started" ) {
            const int cost = camp.time_to_food( 2_hours );
            REQUIRE( cost > 0 );
            const npc_ptr comp = overmap_buffer.find_npc( guy.getID() );
            REQUIRE( comp );
            const mission_id menial{ Camp_Menial, "", {}, base_camps::base_dir };
            REQUIRE( camp.start_mission( menial, 2_hours, true, "", false, {}, std::map<skill_id, int>(),
                                         NO_EXERCISE, 0_hours, comp ) );
            REQUIRE( fac.food_supply().kcal() == 1000 - cost );
            const std::string errors = capture_debugmsg_during( [&camp, &comp]() {
                camp.finish_return( *comp, true, "returns.", skill_menial.str(), 0 );
            } );
            THEN( "return takes nothing more from the larder" ) {
                CAPTURE( errors );
                CHECK( errors.empty() );
                CHECK( fac.food_supply().kcal() == 1000 - cost );
            }
        }
    }
    GIVEN( "larder of 100 kcal" ) {
        stock_larder( fac, 100 );
        WHEN( "500 kcal asked for" ) {
            const camp_food_portion portion = camp.camp_food_supply( -500 );
            THEN( "only what was there is taken, served and counted" ) {
                CHECK( portion.total().kcal() == 100 );
                CHECK( portion.total().get_vitamin( vitamin_iron ) == 10 );
                CHECK( camp.feed_workers( guy, portion, camp_meal::personal ) == 100 );
                CHECK( guy.stomach.get_calories() == 100 );
                CHECK( fac.food_supply().kcal() == 0 );
            }
        }
    }
    GIVEN( "larder of food that keeps and food that expires in a week" ) {
        restore_on_out_of_scope restore_calendar_turn( calendar::turn );
        const time_point expiry = calendar::turn + 7_days;
        stock_larder_with_expiry( fac, expiry );
        guy.add_effect( effect_nausea, 10_minutes );
        WHEN( "nauseous worker refuses to eat the expiring food" ) {
            CHECK( camp.feed_workers( guy, camp.camp_food_supply( -100 ), camp_meal::personal ) == 0 );
            THEN( "the meal returns to the food it came from and expires with it" ) {
                CHECK( stock_expiring_at( fac, expiry ).kcal() == 200 );
                CHECK( stock_expiring_at( fac, expiry ).get_vitamin( vitamin_iron ) == 20 );
                calendar::turn += 14_days;
                CHECK( fac.food_supply().kcal() == 1000 );
            }
        }
        WHEN( "nauseous worker refuses a meal of both kinds" ) {
            CHECK( camp.feed_workers( guy, camp.camp_food_supply( -300 ), camp_meal::personal ) == 0 );
            THEN( "each part returns to the food it came from" ) {
                CHECK( fac.debug_food_supply().size() == 2 );
                CHECK( stock_expiring_at( fac, expiry ).kcal() == 200 );
                CHECK( stock_expiring_at( fac, expiry ).get_vitamin( vitamin_iron ) == 20 );
                CHECK( stock_expiring_at( fac, calendar::turn_zero ).kcal() == 1000 );
                CHECK( stock_expiring_at( fac, calendar::turn_zero ).get_vitamin( vitamin_iron ) == 100 );
            }
        }
        WHEN( "nauseous worker refuses a meal that empties two kinds that expire" ) {
            const time_point later = expiry + 7_days;
            nutrients expires_later;
            expires_later.calories = 100 * 1000;
            expires_later.set_vitamin( vitamin_iron, 10 );
            fac.debug_food_supply().emplace_back( later, expires_later );
            CHECK( camp.feed_workers( guy, camp.camp_food_supply( -350 ), camp_meal::personal ) == 0 );
            THEN( "each part returns to the food it came from" ) {
                CHECK( fac.debug_food_supply().size() == 3 );
                CHECK( stock_expiring_at( fac, calendar::turn_zero ).kcal() == 1000 );
                CHECK( stock_expiring_at( fac, expiry ).kcal() == 200 );
                CHECK( stock_expiring_at( fac, later ).kcal() == 100 );
                CHECK( stock_expiring_at( fac, later ).get_vitamin( vitamin_iron ) == 10 );
            }
        }
        WHEN( "nauseous worker goes on a camp mission" ) {
            const int cost = camp.time_to_food( 2_hours );
            REQUIRE( cost > 0 );
            const npc_ptr comp = overmap_buffer.find_npc( guy.getID() );
            REQUIRE( comp );
            const mission_id menial{ Camp_Menial, "", {}, base_camps::base_dir };
            REQUIRE( camp.start_mission( menial, 2_hours, true, "", false, {}, std::map<skill_id, int>(),
                                         NO_EXERCISE, 0_hours, comp ) );
            THEN( "mission food spent even though worker didn't eat it" ) {
                CHECK( guy.stomach.get_calories() == 0 );
                CHECK( fac.food_supply().kcal() == 1200 - cost );
            }
        }
    }
    GIVEN( "NPC needs off, larder holds food that keeps and food that expires" ) {
        override_option no_npc_food( "NO_NPC_FOOD", "true" );
        const time_point expiry = calendar::turn + 7_days;
        stock_larder_with_expiry( fac, expiry );
        WHEN( "worker goes on a camp mission" ) {
            const int cost = camp.time_to_food( 2_hours );
            REQUIRE( cost > 0 );
            const npc_ptr comp = overmap_buffer.find_npc( guy.getID() );
            REQUIRE( comp );
            const mission_id menial{ Camp_Menial, "", {}, base_camps::base_dir };
            REQUIRE( camp.start_mission( menial, 2_hours, true, "", false, {}, std::map<skill_id, int>(),
                                         NO_EXERCISE, 0_hours, comp ) );
            THEN( "larder still pays for the mission" ) {
                CHECK( fac.food_supply().kcal() == 1200 - cost );
            }
        }
        WHEN( "hungry NPC looks for a meal at camp" ) {
            REQUIRE( guy.has_calorie_deficit() );
            CHECK_FALSE( guy.consume_food_from_camp() );
            THEN( "larder keeps all food, each part has its own expiry" ) {
                CHECK( guy.stomach.get_calories() == 0 );
                CHECK( stock_expiring_at( fac, expiry ).kcal() == 200 );
                CHECK( stock_expiring_at( fac, calendar::turn_zero ).kcal() == 1000 );
            }
        }
    }
    overmap_buffer.clear_camps( camp_omt.xy() );
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
