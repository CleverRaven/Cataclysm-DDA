#include "cata_catch.h"
#include "character_management.h"
#include "game.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "map_selector.h"
#include "overmapbuffer.h"
#include "player_helpers.h"

// A backpack to hold things
static const itype_id itype_backpack( "backpack" );
// The cartridges that go in the mask
static const itype_id itype_gasfilter_med( "gasfilter_med" );
// The gas mask
static const itype_id itype_mask_gas( "mask_gas" );
// Flag identifying cartridges
static const flag_id json_flag_GASFILTER_MED( "GASFILTER_MED" );

// Create a gas mask cartridge with a certain number of charges
static item gas_cartridge_with( int charges )
{
    item ret( itype_gasfilter_med );
    ret.ammo_consume( 100 - charges, tripoint_zero, nullptr );
    REQUIRE( ret.ammo_remaining() == charges );
    return ret;
}

// Equip the character with a backpack to hold things, and wearing an active mask with
// a certain number of charges. Then, spawns a gas field on the character's tile.
// Returns a pointer to the mask
static item *setup_character( Character &guy, int mask_charges )
{
    // First, remove all their items
    guy.worn.clear();

    map &here = get_map();
    item backpack( itype_backpack );
    // Needs to be placed on map to reload into gas mask
    item &cartridge = here.add_item_or_charges( tripoint_zero, gas_cartridge_with( mask_charges ) );
    item gasmask( itype_mask_gas );

    gasmask.reload( guy, { map_cursor( tripoint_zero ), &cartridge}, 1 );
    REQUIRE( gasmask.ammo_remaining() == mask_charges );

    REQUIRE( guy.wear_item( backpack, false ) );

    // Require that the mask is worn, and then have the character activate it
    std::optional<std::list<item>::iterator> mask = guy.wear_item( gasmask, false );
    REQUIRE( mask );
    item *mask_ptr = &( *mask.value() );
    guy.use( {guy, mask_ptr} );
    REQUIRE( mask_ptr->active );

    // Finally, add some gas there for our gas mask to filter out
    REQUIRE( here.add_field( guy.pos(), fd_toxic_gas ) );

    return mask_ptr;
}

// Very simple: Have a gas mask with low charges and a full cartridge.
// Wait in gas until the cartridge is auto-reloaded
static void test_autoreload_works( Character &guy )
{
    // Low charges to ensure change happens soon
    item *gasmask = setup_character( guy, 12 );

    // And a full cartridge to reload with
    guy.i_add( gas_cartridge_with( 100 ) );
    // Then wait in the gas until it's almost out
    do {
        guy.process_items();
        calendar::turn += 1_seconds;
    } while( gasmask->ammo_remaining() > 2 );

    // Ensure we're just about to replace, but not yet at the threshold
    // (we won't hit one because when it hits one, it instantly replaces it)
    CHECK( gasmask->ammo_remaining() == 2 );
    // And check to ensure that the mask is not replaced until entirely necessary
    std::vector<const item *> filters = guy.all_items_with_flag( json_flag_GASFILTER_MED );
    REQUIRE( filters.size() == 1 );
    CHECK( filters[0]->ammo_remaining() == 100 );

    // Then, give some turns to replace the cartridge
    for( int i = 0; i < 10 && gasmask->ammo_remaining() != 100; ++i ) {
        guy.process_items();
    }
    // Ensure it has been replaced in the mask
    CHECK( gasmask->ammo_remaining() == 100 );
    // And that our remaining cartridge has only charge (and there's only one)
    filters = guy.all_items_with_flag( json_flag_GASFILTER_MED );
    REQUIRE( filters.size() == 1 );
    CHECK( filters[0]->ammo_remaining() == 1 );
}

// Check that the "low" mode of autoreload works correctly
static void test_autoreload_low( Character &guy )
{
    // Give them one charge to ensure immediate reload
    item *gasmask = setup_character( guy, 1 );

    // Give them a full cartridge, a partially used cartridge, and two empties
    guy.i_add( gas_cartridge_with( 100 ) );
    guy.i_add( gas_cartridge_with( 50 ) );
    guy.i_add( gas_cartridge_with( 1 ) );
    guy.i_add( gas_cartridge_with( 0 ) );

    // Now, reload the gasmask with the 'least' rule
    char_autoreload::params params;
    params.reload_style = char_autoreload::RELOAD_LEAST;
    guy.try_autoreload( *gasmask, params );

    // 50 should be the only valid choice
    CHECK( gasmask->ammo_remaining() == 50 );
    // And we should have 4 filters on us, 1 with 100, 2 with 1, and 1 with 0
    std::vector<const item *> filters = guy.all_items_with_flag( json_flag_GASFILTER_MED );
    REQUIRE( filters.size() == 4 );
    std::sort( filters.begin(), filters.end(), []( const item * l, const item * r ) {
        return l->ammo_remaining() > r->ammo_remaining();
    } );
    CHECK( filters[0]->ammo_remaining() == 100 );
    CHECK( filters[1]->ammo_remaining() == 1 );
    CHECK( filters[2]->ammo_remaining() == 1 );
    CHECK( filters[3]->ammo_remaining() == 0 );
}

// Check that the "high" mode of autoreload works correctly
static void test_autoreload_high( Character &guy )
{
    // Give them one charge to ensure immediate reload
    item *gasmask = setup_character( guy, 1 );

    // Give them a full cartridge, a partially used cartridge, and two empties
    guy.i_add( gas_cartridge_with( 100 ) );
    guy.i_add( gas_cartridge_with( 50 ) );
    guy.i_add( gas_cartridge_with( 1 ) );
    guy.i_add( gas_cartridge_with( 0 ) );

    // Now, reload the gasmask with the 'least' rule
    char_autoreload::params params;
    params.reload_style = char_autoreload::RELOAD_MOST;
    guy.try_autoreload( *gasmask, params );

    // 100 should be the only valid choice
    CHECK( gasmask->ammo_remaining() == 100 );
    // And we should have 4 filters on us, 1 with 50, 2 with 1, and 1 with 0
    std::vector<const item *> filters = guy.all_items_with_flag( json_flag_GASFILTER_MED );
    REQUIRE( filters.size() == 4 );
    std::sort( filters.begin(), filters.end(), []( const item * l, const item * r ) {
        return l->ammo_remaining() > r->ammo_remaining();
    } );
    CHECK( filters[0]->ammo_remaining() == 50 );
    CHECK( filters[1]->ammo_remaining() == 1 );
    CHECK( filters[2]->ammo_remaining() == 1 );
    CHECK( filters[3]->ammo_remaining() == 0 );
}

static void test_autoreload_return_vals( Character &guy )
{
    // Frequently used
    const char_autoreload::params params;

    // Not low enough
    item *gasmask = setup_character( guy, 40 );
    guy.i_add( gas_cartridge_with( 100 ) );
    guy.i_add( gas_cartridge_with( 1 ) );
    CHECK_FALSE( guy.try_autoreload( *gasmask, params ) );

    // Low, but doesn't need reload
    gasmask = setup_character( guy, 4 );
    guy.i_add( gas_cartridge_with( 100 ) );
    guy.i_add( gas_cartridge_with( 1 ) );
    CHECK_FALSE( guy.try_autoreload( *gasmask, params ) );

    // No valid mags
    gasmask = setup_character( guy, 1 );
    guy.i_add( gas_cartridge_with( 1 ) );
    CHECK_FALSE( guy.try_autoreload( *gasmask, params ) );

    // Reloadable
    gasmask = setup_character( guy, 1 );
    guy.i_add( gas_cartridge_with( 100 ) );
    CHECK( guy.try_autoreload( *gasmask, params ) );
}

TEST_CASE( "auto-reload-basic", "[character][autoreload]" )
{
    clear_map();
    clear_avatar();

    // First, check that the PC can use autoreload
    Character &pc = get_player_character();
    test_autoreload_works( pc );
    test_autoreload_low( pc );
    test_autoreload_high( pc );
    test_autoreload_return_vals( pc );

    // Next, check that an NPC can do it
    shared_ptr_fast<npc> guy = make_shared_fast<npc>();
    guy->normalize();
    guy->randomize();
    guy->spawn_at_precise( tripoint_abs_ms( get_map().getabs( pc.pos() ) ) + point( 1, 1 ) );
    overmap_buffer.insert_npc( guy );
    g->load_npcs();
    clear_character( *guy );
    // And put them on a different tile than the PC (clear_character sets position)
    guy->setpos( pc.pos() + point( 1, 1 ) );

    test_autoreload_works( *guy );
    test_autoreload_low( *guy );
    test_autoreload_high( *guy );
    test_autoreload_return_vals( *guy );
}

TEST_CASE( "auto-reload-messaging", "[character][autoreload]" )
{
    // We'll be using these a lot
    const char_autoreload::params params;
    const std::string fail_var_name = "autoreload_failed_time";
    const std::string warn_var_name = "autoreload_warn_time";

    // Get a PC,
    clear_map();
    clear_avatar();
    Character &pc = get_player_character();

    // And an NPC
    shared_ptr_fast<npc> guy = make_shared_fast<npc>();
    guy->normalize();
    guy->randomize();
    guy->spawn_at_precise( tripoint_abs_ms( get_map().getabs( pc.pos() ) ) + point( 1, 1 ) );
    overmap_buffer.insert_npc( guy );
    g->load_npcs();
    clear_character( *guy );
    // And put them on a different tile than the PC (clear_character sets position)
    guy->setpos( pc.pos() + point( 1, 1 ) );
    Character &npc = *guy;

    SECTION( "Warnings when mask is low" ) {
        // A pc and npc are wearing a gas mask that is low
        item &pcmask = *setup_character( pc, 7 );
        item &npcmask = *setup_character( npc, 7 );

        // Their gas masks are processed
        pc.try_autoreload( pcmask, params );
        npc.try_autoreload( npcmask, params );

        // The PC's mask gives a warning
        CHECK( pcmask.get_var( warn_var_name, calendar::turn - 1_hours ) == calendar::turn );

        // The NPC's mask does not give a warning
        CHECK( npcmask.get_var( warn_var_name, calendar::turn - 1_hours ) != calendar::turn );

        // Seven more turns pass
        for( int i = 0; i < 7; ++i ) {
            calendar::turn += 1_seconds;
            pc.try_autoreload( pcmask, params );
            npc.try_autoreload( npcmask, params );
            // The PC's mask stays silent
            CHECK( pcmask.get_var( warn_var_name, calendar::turn ) != calendar::turn );
            // The NPC's mask stays silent
            CHECK( npcmask.get_var( warn_var_name, calendar::turn - 1_hours ) == calendar::turn - 1_hours );
        }

        // It's eight turns since the message
        calendar::turn += 1_seconds;
        pc.try_autoreload( pcmask, params );
        npc.try_autoreload( npcmask, params );

        // The PC's mask gives a warning
        CHECK( pcmask.get_var( warn_var_name, calendar::turn - 1_hours ) == calendar::turn );

        // The NPC's mask does not give a warning
        CHECK( npcmask.get_var( warn_var_name, calendar::turn - 1_hours ) != calendar::turn );
    }

    SECTION( "Messages on failure to reload" ) {
        // pc and npc with gas masks that have run out
        item &pcmask = *setup_character( pc, 1 );
        item &npcmask = *setup_character( npc, 1 );

        // Both have an empty cartridge on them
        pc.i_add( gas_cartridge_with( 1 ) );
        npc.i_add( gas_cartridge_with( 1 ) );

        // Process the masks
        pc.try_autoreload( pcmask, params );
        npc.try_autoreload( npcmask, params );

        // Both fail to reload
        CHECK( pcmask.get_var( fail_var_name, calendar::turn - 1_hours ) == calendar::turn );
        CHECK( npcmask.get_var( fail_var_name, calendar::turn - 1_hours ) == calendar::turn );

        // For the next 59 turns, they don't try
        for( int i = 0; i < 59; ++i ) {
            calendar::turn += 1_seconds;
            pc.try_autoreload( pcmask, params );
            npc.try_autoreload( npcmask, params );
            CHECK( pcmask.get_var( fail_var_name, calendar::turn ) != calendar::turn );
            CHECK( npcmask.get_var( fail_var_name, calendar::turn ) != calendar::turn );
        }

        // But after a minute, they try again
        calendar::turn += 1_seconds;
        pc.try_autoreload( pcmask, params );
        npc.try_autoreload( npcmask, params );
        CHECK( pcmask.get_var( fail_var_name, calendar::turn - 1_hours ) == calendar::turn );
        CHECK( npcmask.get_var( fail_var_name, calendar::turn - 1_hours ) == calendar::turn );
    }
}
