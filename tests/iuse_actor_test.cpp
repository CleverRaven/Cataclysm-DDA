#include <algorithm>
#include <functional>
#include <iosfwd>
#include <memory>
#include <new>
#include <string>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "colony.h"
#include "game.h"
#include "item.h"
#include "item_contents.h"
#include "item_location.h"
#include "item_pocket.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "map.h"
#include "map_helpers.h"
#include "map_selector.h"
#include "material.h"
#include "monster.h"
#include "mtype.h"
#include "optional.h"
#include "player.h"
#include "player_helpers.h"
#include "point.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"

static monster *find_adjacent_monster( const tripoint &pos )
{
    tripoint target = pos;
    for( target.x = pos.x - 1; target.x <= pos.x + 1; target.x++ ) {
        for( target.y = pos.y - 1; target.y <= pos.y + 1; target.y++ ) {
            if( target == pos ) {
                continue;
            }
            if( monster *const candidate = g->critter_at<monster>( target ) ) {
                return candidate;
            }
        }
    }
    return nullptr;
}

TEST_CASE( "manhack", "[iuse_actor][manhack]" )
{
    clear_avatar();
    player &player_character = get_avatar();
    clear_map();

    g->clear_zombies();
    item &test_item = player_character.i_add( item( "bot_manhack", calendar::turn_zero,
                      item::default_charges_tag{} ) );

    REQUIRE( player_character.has_item( test_item ) );

    monster *new_manhack = find_adjacent_monster( player_character.pos() );
    REQUIRE( new_manhack == nullptr );

    player_character.invoke_item( &test_item );

    REQUIRE( !player_character.has_item_with( []( const item & it ) {
        return it.typeId() == itype_id( "bot_manhack" );
    } ) );

    new_manhack = find_adjacent_monster( player_character.pos() );
    REQUIRE( new_manhack != nullptr );
    REQUIRE( new_manhack->type->id == mtype_id( "mon_manhack" ) );
    g->clear_zombies();
}

TEST_CASE( "tool transform when activated", "[iuse][tool][transform]" )
{
    player &dummy = get_avatar();
    clear_avatar();

    GIVEN( "flashlight with a charged battery installed" ) {
        item flashlight( "flashlight" );
        item bat_cell( "light_battery_cell" );
        REQUIRE( flashlight.is_reloadable_with( itype_id( "light_battery_cell" ) ) );

        // Charge the battery
        const int bat_charges = bat_cell.ammo_capacity( ammotype( "battery" ) );
        bat_cell.ammo_set( bat_cell.ammo_default(), bat_charges );
        REQUIRE( bat_cell.ammo_remaining() == bat_charges );

        // Put battery in flashlight
        REQUIRE( flashlight.has_pocket_type( item_pocket::pocket_type::MAGAZINE_WELL ) );
        ret_val<bool> result = flashlight.put_in( bat_cell, item_pocket::pocket_type::MAGAZINE_WELL );
        REQUIRE( result.success() );
        REQUIRE( flashlight.magazine_current() );

        WHEN( "flashlight is turned on" ) {
            // Activation occurs via the iuse_transform actor
            const use_function *use = flashlight.type->get_use( "transform" );
            REQUIRE( use != nullptr );
            const iuse_transform *actor = dynamic_cast<const iuse_transform *>( use->get_actor_ptr() );
            actor->use( dummy, flashlight, false, dummy.pos() );

            THEN( "it becomes active" ) {
                CHECK( flashlight.active );
            }
            THEN( "the battery remains installed" ) {
                CHECK( flashlight.magazine_current() );
            }
            THEN( "name indicates (on) status" ) {
                CHECK( flashlight.tname() == "flashlight (on)" );
            }
        }
    }
}

static void cut_up_yields( const std::string &target )
{
    map &here = get_map();
    player &guy = get_avatar();
    clear_avatar();
    // Nominal dex to avoid yield penalty.
    guy.dex_cur = 12;
    //guy.set_skill_level( skill_id( "fabrication" ), 10 );
    here.i_at( guy.pos() ).clear();

    CAPTURE( target );
    salvage_actor test_actor;
    item cut_up_target{ target };
    item tool{ "knife_butcher" };
    const std::vector<material_id> &target_materials = cut_up_target.made_of();
    units::mass smallest_yield_mass = units::mass_max;
    for( const material_id &mater : target_materials ) {
        if( const cata::optional<itype_id> item_id = mater->salvaged_into() ) {
            smallest_yield_mass = std::min( smallest_yield_mass, item_id->obj().weight );
        }
    }
    REQUIRE( smallest_yield_mass != units::mass_max );

    units::mass cut_up_target_mass = cut_up_target.weight();
    item &spawned_item = here.add_item_or_charges( guy.pos(), cut_up_target );
    item_location item_loc( map_cursor( guy.pos() ), &spawned_item );

    REQUIRE( smallest_yield_mass <= cut_up_target_mass );

    test_actor.cut_up( guy, tool, item_loc );

    map_stack salvaged_items = here.i_at( guy.pos() );
    units::mass salvaged_mass = 0_gram;
    for( const item &salvage : salvaged_items ) {
        salvaged_mass += salvage.weight();
    }
    CHECK( salvaged_mass <= cut_up_target_mass );
    CHECK( salvaged_mass >= ( cut_up_target_mass * 0.99 ) - smallest_yield_mass );
}

TEST_CASE( "cut_up_yields" )
{
    cut_up_yields( "blanket" );
    cut_up_yields( "backpack_hiking" );
    cut_up_yields( "boxpack" );
    cut_up_yields( "case_violin" );
    cut_up_yields( "down_mattress" );
    cut_up_yields( "plastic_boat_hull" );
    cut_up_yields( "bunker_coat" );
    cut_up_yields( "bunker_pants" );
    cut_up_yields( "kevlar_harness" );
    cut_up_yields( "touring_suit" );
    cut_up_yields( "dress_wedding" );
    cut_up_yields( "wetsuit" );
    cut_up_yields( "wetsuit_booties" );
    cut_up_yields( "wetsuit_hood" );
    cut_up_yields( "wetsuit_spring" );
    cut_up_yields( "wetsuit_gloves" );
    cut_up_yields( "peacoat" );
    cut_up_yields( "log" );
    cut_up_yields( "stick" );
    cut_up_yields( "stick_long" );
    cut_up_yields( "tazer" );
    cut_up_yields( "control_laptop" );
    cut_up_yields( "voltmeter" );
    cut_up_yields( "burette" );
    cut_up_yields( "eink_tablet_pc" );
    cut_up_yields( "camera" );
    cut_up_yields( "cell_phone" );
    cut_up_yields( "laptop" );
    cut_up_yields( "radio" );
    cut_up_yields( "under_armor" );
    cut_up_yields( "acidchitin_harness_dog" );
    cut_up_yields( "chitin_harness_dog" );
    cut_up_yields( "leather_harness_dog" );
    cut_up_yields( "leatherbone_harness_dog" );
    cut_up_yields( "kevlar_harness" );
    cut_up_yields( "rubber_harness_dog" );
}
