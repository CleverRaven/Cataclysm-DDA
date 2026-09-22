#include <array>
#include <climits>
#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "../src/temp_crafting_inventory.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_utility.h"
#include "coordinates.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "npc.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "string_formatter.h"
#include "type_id.h"
#include "units.h"
#include "visitable.h"

class Character;

static const flag_id json_flag_ITEM_BROKEN( "ITEM_BROKEN" );
static const flag_id json_flag_USE_UPS( "USE_UPS" );

static const furn_str_id furn_test_f_reserve_qual( "test_f_reserve_qual" );

static const itype_id itype_UPS( "UPS" );
static const itype_id itype_UPS_ON( "UPS_ON" );
static const itype_id itype_UPS_off( "UPS_off" );
static const itype_id itype_any( "any" );
static const itype_id itype_backpack( "backpack" );
static const itype_id itype_battery_ups( "battery_ups" );
static const itype_id itype_butane( "butane" );
static const itype_id itype_cudgel( "cudgel" );
static const itype_id itype_debug_backpack( "debug_backpack" );
static const itype_id itype_hammer( "hammer" );
static const itype_id itype_knife_hunting( "knife_hunting" );
static const itype_id itype_lighter( "lighter" );
static const itype_id itype_pot( "pot" );
static const itype_id itype_rock( "rock" );
static const itype_id itype_soldering_iron( "soldering_iron" );
static const itype_id itype_test_fire_ax( "test_fire_ax" );
static const itype_id itype_test_gum( "test_gum" );
static const itype_id itype_test_halligan( "test_halligan" );
static const itype_id itype_test_reserve_bionic_rod( "test_reserve_bionic_rod" );
static const itype_id itype_water( "water" );

static const quality_id qual_AXE( "AXE" );
static const quality_id qual_BOIL( "BOIL" );
static const quality_id qual_CUT( "CUT" );
static const quality_id qual_DIG( "DIG" );
static const quality_id qual_FISHING_ROD( "FISHING_ROD" );
static const quality_id qual_HAMMER( "HAMMER" );
static const quality_id qual_PRY( "PRY" );

static const tripoint_bub_ms pile_origin( 60, 60, 0 );

TEST_CASE( "temp_crafting_inv_test_amount", "[crafting][inventory]" )
{
    temp_crafting_inventory inv;
    CHECK( inv.size() == 0 );

    item gum( itype_test_gum, calendar::turn_zero, item::default_charges_tag{} );

    inv.add_item_ref( gum );
    CHECK( inv.size() == 1 );

    CHECK( inv.amount_of( itype_test_gum ) == 1 );
    CHECK( inv.has_amount( itype_test_gum, 1 ) );
    CHECK( inv.has_charges( itype_test_gum, 10 ) );
    CHECK_FALSE( inv.has_charges( itype_test_gum, 11 ) );

    inv.clear();
    CHECK( inv.size() == 0 );
}

TEST_CASE( "temp_crafting_inv_test_quality", "[crafting][inventory]" )
{
    temp_crafting_inventory inv;
    inv.add_item_copy( item( itype_test_halligan ) );

    CHECK( inv.has_quality( qual_HAMMER, 1 ) );
    CHECK( inv.has_quality( qual_HAMMER, 2 ) );
    CHECK_FALSE( inv.has_quality( qual_HAMMER, 3 ) );
    CHECK( inv.has_quality( qual_DIG, 1 ) );
    CHECK_FALSE( inv.has_quality( qual_AXE ) );

    inv.add_item_copy( item( itype_test_fire_ax ) );
    CHECK( inv.has_quality( qual_AXE ) );

    CHECK( inv.max_quality( qual_PRY ) == 4 );
}

static item charged_ups( const itype_id &type, int charges )
{
    item ups( type );
    item mag( ups.magazine_default() );
    mag.ammo_set( mag.ammo_default(), charges );
    REQUIRE( ups.put_in( mag, pocket_type::MAGAZINE_WELL ).success() );
    return ups;
}

// one pile covering each way a subtree can hold a type, with every kind of top-level entry.
static void build_pile( temp_crafting_inventory &inv, item &loose )
{
    map &here = get_map();

    item nested_packs( itype_debug_backpack );
    REQUIRE( nested_packs.put_in( item( itype_debug_backpack ), pocket_type::CONTAINER ).success() );
    here.add_item( pile_origin, nested_packs );

    item knives( itype_backpack );
    REQUIRE( knives.put_in( item( itype_knife_hunting ), pocket_type::CONTAINER ).success() );
    REQUIRE( knives.put_in( item( itype_knife_hunting ), pocket_type::CONTAINER ).success() );
    here.add_item( pile_origin, knives );

    here.add_item( pile_origin, tool_with_ammo( itype_lighter, 10 ) );

    here.add_item( pile_origin, charged_ups( itype_UPS_ON, 100 ) );
    item ups_bag( itype_backpack );
    REQUIRE( ups_bag.put_in( charged_ups( itype_UPS_off, 50 ), pocket_type::CONTAINER ).success() );
    here.add_item( pile_origin, ups_bag );
    for( int i = 0; i < 2; ++i ) {
        item iron( itype_soldering_iron );
        REQUIRE( iron.put_in( item( itype_battery_ups ), pocket_type::MOD ).success() );
        REQUIRE( iron.has_flag( json_flag_USE_UPS ) );
        here.add_item( pile_origin, iron );
    }

    item broken( itype_hammer );
    broken.set_flag( json_flag_ITEM_BROKEN );
    REQUIRE( broken.is_broken() );
    here.add_item( pile_origin, broken );
    here.furn_set( pile_origin, furn_test_f_reserve_qual );

    inv.form_from_map( pile_origin, 0, nullptr, false );
    inv.add_item_ref( loose );
    inv.add_item_copy( item( itype_knife_hunting ) );
}

static std::map<std::string, int> ask_everything( const temp_crafting_inventory &inv,
        const std::set<itype_id> &types )
{
    const std::array<bool, 2> pseudo_modes{ true, false };
    const std::array<int, 4> limits{ 0, 1, 2, INT_MAX };
    std::map<std::string, int> out;
    for( const itype_id &type : types ) {
        for( const bool pseudo : pseudo_modes ) {
            for( const int limit : limits ) {
                out[string_format( "amount_of %s pseudo %d limit %d", type.str(), pseudo ? 1 : 0,
                                   limit )] = inv.amount_of( type, pseudo, limit );
            }
        }
        out["charges_of " + type.str()] = inv.charges_of( type );
        out["count_item " + type.str()] = inv.count_item( type );
    }
    out["charges_of UPS"] = inv.charges_of( itype_UPS );
    int drawn = 0;
    out["charges_of soldering_iron"] = inv.charges_of( itype_soldering_iron, INT_MAX,
    return_true<item>, [&drawn]( int used ) {
        drawn += used;
    } );
    out["UPS drawn by soldering_iron"] = drawn;
    out["charges_of butane in tools"] = inv.charges_of( itype_butane, INT_MAX, return_true<item>,
                                        nullptr, true );
    out["amount_of any"] = inv.amount_of( itype_any );
    return out;
}

TEST_CASE( "temp_crafting_inventory_index_answers_like_a_live_walk", "[crafting][inventory]" )
{
    clear_avatar();
    clear_map();
    item loose( itype_hammer );
    temp_crafting_inventory inv;
    build_pile( inv, loose );

    std::set<itype_id> types{ itype_cudgel, itype_rock };
    inv.visit_items( [&types]( const item * node, item * ) {
        types.insert( node->typeId() );
        return VisitResponse::NEXT;
    } );
    REQUIRE( inv.amount_of( itype_cudgel ) == 0 );
    REQUIRE( inv.amount_of( itype_rock ) == 0 );

    const std::map<std::string, int> live = ask_everything( inv, types );
    CHECK( live.at( "charges_of UPS" ) == 150 );
    CHECK( live.at( "UPS drawn by soldering_iron" ) == live.at( "charges_of UPS" ) );

    temp_crafting_inventory::query_cache_scope scope;
    for( int pass = 0; pass < 2; ++pass ) {
        const std::map<std::string, int> cached = ask_everything( inv, types );
        for( const auto &[query, answer] : live ) {
            CAPTURE( pass, query );
            CHECK( cached.at( query ) == answer );
        }
    }
}

TEST_CASE( "temp_crafting_inventory_query_cache_lifecycle", "[crafting][inventory]" )
{
    clear_avatar();
    clear_map();
    item &box = get_map().add_item( pile_origin, item( itype_debug_backpack ) );
    REQUIRE_FALSE( box.is_null() );
    temp_crafting_inventory inv;
    inv.form_from_map( pile_origin, 0, nullptr, false );
    const auto sneak_in_a_knife = [&box]() {
        REQUIRE( box.put_in( item( itype_knife_hunting ), pocket_type::CONTAINER ).success() );
    };
    REQUIRE( inv.amount_of( itype_knife_hunting ) == 0 );

    SECTION( "cache answers inside a scope and every mutator drops it" ) {
        temp_crafting_inventory::query_cache_scope scope;
        REQUIRE( inv.amount_of( itype_knife_hunting ) == 0 );
        sneak_in_a_knife();
        CHECK( inv.amount_of( itype_knife_hunting ) == 0 );
        inv.add_item_copy( item( itype_knife_hunting ) );
        CHECK( inv.amount_of( itype_knife_hunting ) == 2 );
        inv.remove_items_with( []( const item & it ) {
            return it.typeId() == itype_knife_hunting;
        } );
        CHECK( inv.amount_of( itype_knife_hunting ) == 1 );
        inv.clear();
        CHECK( inv.amount_of( itype_knife_hunting ) == 0 );
        inv.form_from_map( pile_origin, 0, nullptr, false );
        CHECK( inv.amount_of( itype_knife_hunting ) == 1 );
    }
    SECTION( "assignment replaces a populated cache" ) {
        temp_crafting_inventory::query_cache_scope scope;
        temp_crafting_inventory other;
        other.add_item_copy( item( itype_knife_hunting ) );
        REQUIRE( inv.amount_of( itype_knife_hunting ) == 0 );
        inv = other;
        CHECK( inv.amount_of( itype_knife_hunting ) == 1 );
    }
    SECTION( "a copy answers from its own items" ) {
        temp_crafting_inventory::query_cache_scope scope;
        std::unique_ptr<temp_crafting_inventory> source = std::make_unique<temp_crafting_inventory>();
        source->add_item_copy( item( itype_knife_hunting ) );
        REQUIRE( source->amount_of( itype_knife_hunting ) == 1 );
        const temp_crafting_inventory copy( *source );
        source.reset();
        CHECK( copy.amount_of( itype_knife_hunting ) == 1 );
    }
    SECTION( "quality answer is remembered from the first query of a scope" ) {
        temp_crafting_inventory::query_cache_scope scope;
        REQUIRE_FALSE( inv.has_provider_quality( qual_CUT, 1, 1, nullptr ) );
        sneak_in_a_knife();
        CHECK_FALSE( inv.has_provider_quality( qual_CUT, 1, 1, nullptr ) );
    }
    SECTION( "boiling answer does not outlive its scope" ) {
        REQUIRE( box.put_in( item( itype_pot ), pocket_type::CONTAINER ).success() );
        item &pot = *box.all_items_top( pocket_type::CONTAINER ).front();
        {
            temp_crafting_inventory::query_cache_scope first;
            REQUIRE( inv.has_provider_quality( qual_BOIL, 1, 1, nullptr ) );
        }
        REQUIRE( pot.put_in( item( itype_water, calendar::turn, 1 ),
                             pocket_type::CONTAINER ).success() );
        temp_crafting_inventory::query_cache_scope second;
        CHECK_FALSE( inv.has_provider_quality( qual_BOIL, 1, 1, nullptr ) );
    }
    SECTION( "cache lives only as long as its outermost scope" ) {
        {
            temp_crafting_inventory::query_cache_scope outer;
            REQUIRE( inv.amount_of( itype_knife_hunting ) == 0 );
            sneak_in_a_knife();
            {
                temp_crafting_inventory::query_cache_scope nested;
            }
            CHECK( inv.amount_of( itype_knife_hunting ) == 0 );
        }
        CHECK( inv.amount_of( itype_knife_hunting ) == 1 );
        sneak_in_a_knife();
        temp_crafting_inventory::query_cache_scope later;
        CHECK( inv.amount_of( itype_knife_hunting ) == 2 );
    }
}

namespace
{
struct quality_ask {
    quality_id qual;
    int level;
    int qty;
    const Character *who;
    quality_count mode;
};
} // namespace

static bool ask( const temp_crafting_inventory &inv, const quality_ask &q )
{
    return inv.has_provider_quality( q.qual, q.level, q.qty, q.who, q.mode );
}

TEST_CASE( "provider_quality_memo_keeps_every_key_dimension", "[crafting][inventory]" )
{
    clear_avatar();
    avatar &u = get_avatar();
    u.set_max_power_level( 10_kJ );
    u.set_power_level( 10_kJ );
    const standard_npc unpowered( "Unpowered" );
    REQUIRE( unpowered.get_power_level() == 0_kJ );

    temp_crafting_inventory inv;
    inv.add_item_copy( item( itype_test_reserve_bionic_rod ) );
    inv.add_item_copy( item( itype_test_halligan ) );
    inv.add_item_copy( item( itype_rock, calendar::turn, 2 ) );

    const quality_count each = quality_count::providers;
    const quality_count units = quality_count::units;
    // two queries of a pair differ in one key dimension and in their answer
    const std::vector<std::pair<quality_ask, quality_ask>> pairs = {
        { { qual_FISHING_ROD, 1, 1, &u, each }, { qual_FISHING_ROD, 1, 1, &unpowered, each } },
        { { qual_FISHING_ROD, 1, 1, &u, each }, { qual_FISHING_ROD, 1, 1, nullptr, each } },
        { { qual_HAMMER, 2, 1, nullptr, each }, { qual_HAMMER, 3, 1, nullptr, each } },
        { { qual_PRY, 1, 1, nullptr, each }, { qual_PRY, 1, 2, nullptr, each } },
        { { qual_DIG, 1, 1, nullptr, each }, { qual_AXE, 1, 1, nullptr, each } },
        { { qual_HAMMER, 1, 3, nullptr, units }, { qual_HAMMER, 1, 3, nullptr, each } },
    };
    std::vector<quality_ask> asks;
    for( const auto &[first, second] : pairs ) {
        CAPTURE( asks.size() );
        REQUIRE( ask( inv, first ) != ask( inv, second ) );
        asks.push_back( first );
        asks.push_back( second );
    }
    std::vector<bool> live;
    live.reserve( asks.size() );
    for( const quality_ask &q : asks ) {
        live.push_back( ask( inv, q ) );
    }

    const std::array<bool, 2> orders{ false, true };
    for( const bool reverse : orders ) {
        temp_crafting_inventory::query_cache_scope scope;
        for( int pass = 0; pass < 2; ++pass ) {
            for( size_t n = 0; n < asks.size(); ++n ) {
                const size_t i = reverse ? asks.size() - 1 - n : n;
                CAPTURE( reverse, pass, i );
                CHECK( ask( inv, asks[i] ) == live[i] );
            }
        }
    }
}
