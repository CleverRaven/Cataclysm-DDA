#include "avatar.h"
#include "cata_catch.h"
#include "itype.h"
#include "map_selector.h"
#include "npc.h"
#include "npctrade.h"
#include "player_helpers.h"

static const skill_id skill_speech( "speech" );

TEST_CASE( "basic_price_check", "[npc][trade]" )
{
    using namespace npc_trading;
    clear_avatar();
    avatar &u = get_avatar();
    npc &guy = spawn_npc( { 50, 50 }, "test_npc_trader" );
    clear_character( guy );

    bool const u_buy = GENERATE( true, false );
    CAPTURE( u_buy );

    Character *seller = nullptr;
    Character *buyer = nullptr;
    if( u_buy ) {
        buyer = &u;
        seller = &guy;
    } else {
        buyer = &guy;
        seller = &u;
    }

    item m4( "modular_m4_carbine" );
    item mag( m4.magazine_default() );
    int const ammo_amount = mag.remaining_ammo_capacity();
    item ammo( mag.ammo_default(), calendar::turn, ammo_amount );
    item bomba( "test_bomba" );
    REQUIRE( bomba.type->price_post != units::from_cent( 0 ) );
    item backpack( "debug_backpack" );

    int const price_separate = adjusted_price( &m4, 1, *buyer, *seller ) +
                               adjusted_price( &mag, 1, *buyer, *seller ) +
                               adjusted_price( &ammo, ammo_amount, *buyer, *seller ) +
                               adjusted_price( &backpack, 1, *buyer, *seller );

    mag.put_in( ammo, pocket_type::MAGAZINE );
    m4.put_in( mag, pocket_type::MAGAZINE_WELL );
    backpack.put_in( m4, pocket_type::CONTAINER );
    if( !u_buy ) {
        REQUIRE( !guy.wants_to_buy( bomba ) );
        backpack.put_in( bomba, pocket_type::CONTAINER );
    }

    trade_selector::entry_t bck_entry{
        item_location{ map_cursor( tripoint_zero ), &backpack }, 1 };

    int const price_combined = trading_price( *buyer, *seller, bck_entry );

    CHECK( price_separate == Approx( price_combined ).margin( 1 ) );
}

TEST_CASE( "faction_price_rules", "[npc][factions][trade]" )
{
    clear_avatar();
    npc &guy = spawn_npc( { 50, 50 }, "test_npc_trader" );
    faction const &fac = *guy.my_fac;

    WHEN( "item has no rules (default adjustment)" ) {
        item const hammer( "hammer" );
        clear_character( guy );
        REQUIRE( npc_trading::adjusted_price( &hammer, 1, get_avatar(), guy ) ==
                 Approx( units::to_cent( hammer.type->price_post ) * 1.25 ).margin( 1 ) );
        REQUIRE( npc_trading::adjusted_price( &hammer, 1, guy, get_avatar() ) ==
                 Approx( units::to_cent( hammer.type->price_post ) * 0.75 ).margin( 1 ) );
    }

    WHEN( "item is main currency (implicit price rule)" ) {
        guy.int_max = 1000;
        guy.set_skill_level( skill_speech, 10 );
        item const fmcnote( "FMCNote" );
        REQUIRE( npc_trading::adjusted_price( &fmcnote, 1, get_avatar(), guy ) ==
                 units::to_cent( fmcnote.type->price_post ) );
        REQUIRE( npc_trading::adjusted_price( &fmcnote, 1, guy, get_avatar() ) ==
                 units::to_cent( fmcnote.type->price_post ) );
    }

    item const pants_fur( "test_pants_fur" );
    WHEN( "item is secondary currency (fixed_adj=0)" ) {
        get_avatar().int_max = 1000;
        get_avatar().set_skill_level( skill_speech, 10 );
        REQUIRE( npc_trading::adjusted_price( &pants_fur, 1, get_avatar(), guy ) ==
                 units::to_cent( pants_fur.type->price_post ) );
        REQUIRE( npc_trading::adjusted_price( &pants_fur, 1, guy, get_avatar() ) ==
                 units::to_cent( pants_fur.type->price_post ) );
    }
    WHEN( "faction desperately needs this item (premium=25)" ) {
        item const multitool( "test_multitool" );
        REQUIRE( fac.get_price_rules( multitool, guy )->premium == 25 );
        REQUIRE( fac.get_price_rules( multitool, guy )->markup == 1.1 );
        THEN( "NPC selling to avatar includes premium and markup" ) {
            REQUIRE( npc_trading::adjusted_price( &multitool, 1, get_avatar(), guy ) ==
                     Approx( units::to_cent( multitool.type->price_post ) * 25 * 1.1 ).margin( 1 ) );
        }
        THEN( "avatar selling to NPC includes only premium" ) {
            REQUIRE( npc_trading::adjusted_price( &multitool, 1, guy, get_avatar() ) ==
                     Approx( units::to_cent( multitool.type->price_post ) * 25 ).margin( 1 ) );
        }
    }
    WHEN( "faction has a custom price for this item (price=10000000)" ) {
        item const log = GENERATE( item( "log" ), item( "scrap" ) );
        clear_character( guy );
        double price = *fac.get_price_rules( log, guy )->price;
        REQUIRE( price == 10000000 );
        if( log.count_by_charges() ) {
            price /= log.type->stack_size;
        }
        REQUIRE( npc_trading::adjusted_price( &log, 1, get_avatar(), guy ) ==
                 Approx( price * 1.25 ).margin( 1 ) );
        REQUIRE( npc_trading::adjusted_price( &log, 1, guy, get_avatar() ) ==
                 Approx( price * 0.75 ).margin( 1 ) );
    }
    item const carafe( "test_nuclear_carafe" );
    WHEN( "condition for price rules not satisfied" ) {
        clear_character( guy );
        REQUIRE( fac.get_price_rules( carafe, guy ) == nullptr );
        REQUIRE( npc_trading::adjusted_price( &carafe, 1, get_avatar(), guy ) ==
                 Approx( units::to_cent( carafe.type->price_post ) * 1.25 ).margin( 1 ) );
    }
    WHEN( "condition for price rules satisfied" ) {
        guy.set_value( "npctalk_var_bool_allnighter_thirsty", "yes" );
        REQUIRE( fac.get_price_rules( carafe, guy )->markup == 2.0 );
        THEN( "NPC selling to avatar includes markup and positive fixed adjustment" ) {
            REQUIRE( npc_trading::adjusted_price( &carafe, 1, get_avatar(), guy ) ==
                     Approx( units::to_cent( carafe.type->price_post ) * 2.0 * 1.1 ).margin( 1 ) );
        }
        THEN( "avatar selling to NPC includes only negative fixed adjustment" ) {
            REQUIRE( npc_trading::adjusted_price( &carafe, 1, guy, get_avatar() ) ==
                     Approx( units::to_cent( carafe.type->price_post ) * 0.9 ).margin( 1 ) );
        }
    }
    WHEN( "personal price rule overrides faction rule" ) {
        double const fmarkup = fac.get_price_rules( pants_fur, guy )->markup;
        REQUIRE( guy.get_price_rules( pants_fur )->markup == fmarkup );
        REQUIRE( fmarkup != - 100 );
        guy.set_value( "npctalk_var_bool_preference_vegan", "yes" );
        REQUIRE( guy.get_price_rules( pants_fur )->markup == -100 );
    }
    WHEN( "price rule affects magazine contents" ) {
        clear_character( guy );
        item const battery( "battery" );
        item tbd( "test_battery_disposable" );
        int const battery_price = *guy.get_price_rules( battery )->price;
        REQUIRE( battery.price( true ) != battery_price );
        trade_selector::entry_t tbd_entry{
            item_location{ map_cursor( tripoint_zero ), &tbd }, 1 };

        REQUIRE( npc_trading::trading_price( get_avatar(), guy, tbd_entry ) ==
                 Approx( units::to_cent( tbd.type->price_post ) * 1.25 +
                         battery_price * 1.25 * tbd.ammo_remaining( nullptr ) / battery.type->stack_size )
                 .margin( 1 ) );
    }
}
