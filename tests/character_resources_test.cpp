#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "avatar.h"
#include "cata_catch.h"
#include "character.h"
#include "flexbuffer_json.h"
#include "item.h"
#include "json_loader.h"
#include "magic.h"
#include "magic_type.h"
#include "pimpl.h"
#include "player_helpers.h"
#include "recipe.h"
#include "string_formatter.h"
#include "type_id.h"
#include "vitamin.h"

static const itype_id itype_stick( "stick" );
static const vitamin_id vitamin_blood( "blood" );

static recipe load_test_recipe( const std::string &character_resources )
{
    const std::string json = string_format( R"({
        "type": "recipe",
        "result": "cudgel",
        "id_suffix": "character_resources_test",
        "category": "CC_WEAPON",
        "subcategory": "CSC_WEAPON_BASHING",
        "skill_used": "fabrication",
        "difficulty": 0,
        "time": "1 m",
        "activity_level": "MODERATE_EXERCISE",
        "components": [ [ [ "stick", 1 ] ] ],
        "character_resources": %s
    })", character_resources );

    const JsonObject jo = json_loader::from_string( json );
    recipe result;
    result.load( jo, "dda" );
    result.finalize();
    return result;
}

static recipe load_base_then_child( const std::string &base_resources,
                                    const std::string &child_resources )
{
    recipe result;

    {
        const std::string json = string_format( R"({
            "type": "recipe",
            "result": "cudgel",
            "id_suffix": "character_resources_base",
            "category": "CC_WEAPON",
            "subcategory": "CSC_WEAPON_BASHING",
            "skill_used": "fabrication",
            "difficulty": 0,
            "time": "1 m",
            "activity_level": "MODERATE_EXERCISE",
            "components": [ [ [ "stick", 1 ] ] ],
            "character_resources": %s
        })", base_resources );

        const JsonObject jo = json_loader::from_string( json );
        jo.allow_omitted_members();
        result.load( jo, "dda" );
    }

    result.was_loaded = true;

    {
        const std::string json = string_format( R"({
            "type": "recipe",
            "result": "cudgel",
            "id_suffix": "character_resources_child",
            "components": [ [ [ "stick", 1 ] ] ],
            "character_resources": %s
        })", child_resources );

        const JsonObject jo = json_loader::from_string( json );
        jo.allow_omitted_members();
        result.load( jo, "dda" );
    }

    result.finalize();
    return result;
}

static void set_vitamin_level( Character &who, const vitamin_id &vitamin, int value )
{
    who.vitamin_mod( vitamin, value - who.vitamin_get( vitamin ) );
}

static item make_test_craft( const recipe &rec, int batch = 1 )
{
    item component( itype_stick );
    return item( &rec, batch, component );
}

TEST_CASE( "recipe_character_resources_load_and_replace_inherited_costs",
           "[recipe][crafting][character_resources]" )
{
    SECTION( "different resource kinds can be combined" ) {
        const recipe rec = load_test_recipe( R"({
            "mana": 1000,
            "stamina": 1000,
            "vitamins": [ { "vitamin": "blood", "value": 10000, "safe_level": -20000 } ]
        })" );

        const character_resource_costs &resources = rec.get_character_resources();

        REQUIRE( resources.energy.size() == 2 );
        CHECK( resources.energy.at( magic_energy_type::mana ) == 1000 );
        CHECK( resources.energy.at( magic_energy_type::stamina ) == 1000 );

        REQUIRE( resources.vitamins.size() == 1 );
        CHECK( resources.vitamins.front().vitamin == vitamin_blood );
        CHECK( resources.vitamins.front().value == 10000 );
        REQUIRE( resources.vitamins.front().safe_level.has_value() );
        CHECK( *resources.vitamins.front().safe_level == -20000 );
    }

    SECTION( "a child object replaces the entire inherited resource block" ) {
        const recipe rec = load_base_then_child(
                               R"({ "mana": 100, "stamina": 200 })",
                               R"({ "vitamins": [ { "vitamin": "blood", "value": 10000, "safe_level": -20000 } ] })"
                           );

        const character_resource_costs &resources = rec.get_character_resources();

        CHECK( resources.energy.empty() );
        REQUIRE( resources.vitamins.size() == 1 );
        CHECK( resources.vitamins.front().vitamin == vitamin_blood );
        CHECK( resources.vitamins.front().value == 10000 );
        REQUIRE( resources.vitamins.front().safe_level.has_value() );
        CHECK( *resources.vitamins.front().safe_level == -20000 );
    }
}

TEST_CASE( "craft_character_resources_follow_progress_and_batch_size",
           "[crafting][character_resources]" )
{
    avatar &you = get_avatar();
    clear_avatar();

    const recipe rec = load_test_recipe( R"({
        "mana": 1000,
        "stamina": 1000,
        "vitamins": [ { "vitamin": "blood", "value": 10000, "safe_level": -20000 } ]
    })" );

    const int initial_mana = you.magic->max_mana( you );
    const int initial_stamina = you.get_stamina_max();
    you.magic->set_mana( initial_mana );
    you.set_stamina( initial_stamina );
    set_vitamin_level( you, vitamin_blood, 0 );

    item craft = make_test_craft( rec, 2 );

    SECTION( "a dry run checks availability without changing state" ) {
        CHECK( you.craft_consume_character_resources( craft, 2500000, false ) );
        CHECK( you.magic->available_mana() == initial_mana );
        CHECK( you.get_stamina() == initial_stamina );
        CHECK( you.vitamin_get( vitamin_blood ) == 0 );
    }

    SECTION( "only the newly reached fraction is consumed" ) {
        REQUIRE( you.craft_consume_character_resources( craft, 2500000 ) );
        CHECK( you.magic->available_mana() == initial_mana - 500 );
        CHECK( you.get_stamina() == initial_stamina - 500 );
        CHECK( you.vitamin_get( vitamin_blood ) == -5000 );

        // Repeating the same target must not charge the craft twice.
        REQUIRE( you.craft_consume_character_resources( craft, 2500000 ) );
        CHECK( you.magic->available_mana() == initial_mana - 500 );
        CHECK( you.get_stamina() == initial_stamina - 500 );
        CHECK( you.vitamin_get( vitamin_blood ) == -5000 );

        REQUIRE( you.craft_consume_character_resources( craft, 5000000 ) );
        CHECK( you.magic->available_mana() == initial_mana - 1000 );
        CHECK( you.get_stamina() == initial_stamina - 1000 );
        CHECK( you.vitamin_get( vitamin_blood ) == -10000 );
    }
}

TEST_CASE( "craft_character_resource_debits_are_atomic",
           "[crafting][character_resources]" )
{
    avatar &you = get_avatar();
    clear_avatar();

    const recipe rec = load_test_recipe( R"({
        "mana": 1000,
        "stamina": 1000,
        "vitamins": [ { "vitamin": "blood", "value": 10000, "safe_level": -20000 } ]
    })" );

    you.magic->set_mana( 1000 );
    you.set_stamina( 999 );
    set_vitamin_level( you, vitamin_blood, 0 );

    item craft = make_test_craft( rec );

    CHECK_FALSE( you.craft_consume_character_resources( craft, 10000000 ) );
    CHECK( you.magic->available_mana() == 1000 );
    CHECK( you.get_stamina() == 999 );
    CHECK( you.vitamin_get( vitamin_blood ) == 0 );
}

TEST_CASE( "craft_vitamin_availability_respects_the_safe_floor",
           "[crafting][character_resources]" )
{
    avatar &you = get_avatar();
    clear_avatar();
    set_vitamin_level( you, vitamin_blood, -15000 );

    vitamin_resource_cost resource;
    resource.vitamin = vitamin_blood;

    SECTION( "an explicit safe level is used" ) {
        resource.safe_level = -20000;
        CHECK( you.craft_vitamin_available( resource ) == 5000 );
    }

    SECTION( "the vitamin minimum is the default safe level" ) {
        resource.safe_level.reset();
        CHECK( you.craft_vitamin_available( resource ) ==
               you.vitamin_get( vitamin_blood ) - vitamin_blood->min() );
    }

    SECTION( "nothing is available below the selected floor" ) {
        resource.safe_level = -10000;
        CHECK( you.craft_vitamin_available( resource ) == 0 );
    }
}