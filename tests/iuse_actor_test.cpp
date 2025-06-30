#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "creature_tracker.h"
#include "game.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "map.h"
#include "map_helpers.h"
#include "map_selector.h"
#include "material.h"
#include "monster.h"
#include "mtype.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"

static const ammotype ammo_battery( "battery" );

static const itype_id itype_acidchitin_harness_dog( "acidchitin_harness_dog" );
static const itype_id itype_backpack_hiking( "backpack_hiking" );
static const itype_id itype_blanket( "blanket" );
static const itype_id itype_bot_manhack( "bot_manhack" );
static const itype_id itype_boxpack( "boxpack" );
static const itype_id itype_bunker_coat( "bunker_coat" );
static const itype_id itype_bunker_pants( "bunker_pants" );
static const itype_id itype_burette( "burette" );
static const itype_id itype_camera( "camera" );
static const itype_id itype_case_violin( "case_violin" );
static const itype_id itype_cell_phone( "cell_phone" );
static const itype_id itype_chitin_harness_dog( "chitin_harness_dog" );
static const itype_id itype_down_mattress( "down_mattress" );
static const itype_id itype_dress_wedding( "dress_wedding" );
static const itype_id itype_eink_tablet_pc( "eink_tablet_pc" );
static const itype_id itype_flashlight( "flashlight" );
static const itype_id itype_kevlar_harness( "kevlar_harness" );
static const itype_id itype_knife_huge( "knife_huge" );
static const itype_id itype_laptop( "laptop" );
static const itype_id itype_leather_harness_dog( "leather_harness_dog" );
static const itype_id itype_leatherbone_harness_dog( "leatherbone_harness_dog" );
static const itype_id itype_log( "log" );
static const itype_id itype_medium_battery_cell( "medium_battery_cell" );
static const itype_id itype_peacoat( "peacoat" );
static const itype_id itype_plastic_boat_hull( "plastic_boat_hull" );
static const itype_id itype_radio( "radio" );
static const itype_id itype_rubber_harness_dog( "rubber_harness_dog" );
static const itype_id itype_stick( "stick" );
static const itype_id itype_stick_long( "stick_long" );
static const itype_id itype_tazer( "tazer" );
static const itype_id itype_touring_suit( "touring_suit" );
static const itype_id itype_under_armor( "under_armor" );
static const itype_id itype_voltmeter( "voltmeter" );
static const itype_id itype_wetsuit( "wetsuit" );
static const itype_id itype_wetsuit_booties( "wetsuit_booties" );
static const itype_id itype_wetsuit_gloves( "wetsuit_gloves" );
static const itype_id itype_wetsuit_hood( "wetsuit_hood" );
static const itype_id itype_wetsuit_spring( "wetsuit_spring" );

static const mtype_id mon_manhack( "mon_manhack" );

static monster *find_adjacent_monster( const tripoint_bub_ms &pos )
{
    tripoint_bub_ms target = pos;
    creature_tracker &creatures = get_creature_tracker();
    for( target.x() = pos.x() - 1; target.x() <= pos.x() + 1; target.x()++ ) {
        for( target.y() = pos.y() - 1; target.y() <= pos.y() + 1; target.y()++ ) {
            if( target == pos ) {
                continue;
            }
            if( monster *const candidate = creatures.creature_at<monster>( target ) ) {
                return candidate;
            }
        }
    }
    return nullptr;
}

TEST_CASE( "manhack", "[iuse_actor][manhack]" )
{
    clear_avatar();
    Character &player_character = get_avatar();
    clear_map();

    g->clear_zombies();
    item_location test_item = player_character.i_add( item( itype_bot_manhack, calendar::turn_zero,
                              item::default_charges_tag{} ) );

    REQUIRE( player_character.has_item( *test_item ) );

    monster *new_manhack = find_adjacent_monster( player_character.pos_bub() );
    REQUIRE( new_manhack == nullptr );

    player_character.invoke_item( &*test_item );

    REQUIRE( !player_character.has_item_with( []( const item & it ) {
        return it.typeId() == itype_bot_manhack;
    } ) );

    new_manhack = find_adjacent_monster( player_character.pos_bub() );
    REQUIRE( new_manhack != nullptr );
    REQUIRE( new_manhack->type->id == mon_manhack );
    g->clear_zombies();
}

TEST_CASE( "tool_transform_when_activated", "[iuse][tool][transform]" )
{
    Character *dummy = &get_avatar();
    clear_avatar();

    GIVEN( "flashlight with a charged battery installed" ) {
        item flashlight( itype_flashlight );
        item bat_cell( itype_medium_battery_cell );
        REQUIRE( flashlight.can_reload_with( item( itype_medium_battery_cell ), true ) );

        // Charge the battery
        const int bat_charges = bat_cell.ammo_capacity( ammo_battery );
        bat_cell.ammo_set( bat_cell.ammo_default(), bat_charges );
        REQUIRE( bat_cell.ammo_remaining( ) == bat_charges );

        // Put battery in flashlight
        REQUIRE( flashlight.has_pocket_type( pocket_type::MAGAZINE_WELL ) );
        ret_val<void> result = flashlight.put_in( bat_cell, pocket_type::MAGAZINE_WELL );
        REQUIRE( result.success() );
        REQUIRE( flashlight.magazine_current() );

        WHEN( "flashlight is turned on" ) {
            // Activation occurs via the iuse_transform actor
            const use_function *use = flashlight.type->get_use( "transform" );
            REQUIRE( use != nullptr );
            const iuse_transform *actor = dynamic_cast<const iuse_transform *>( use->get_actor_ptr() );
            actor->use( dummy, flashlight, dummy->pos_bub() );

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

static void cut_up_yields( const itype_id &target )
{
    map &here = get_map();
    Character &guy = get_avatar();
    clear_avatar();
    // Nominal dex to avoid yield penalty.
    guy.dex_cur = 12;
    //guy.set_skill_level( skill_id( "fabrication" ), 10 );
    here.i_at( guy.pos_bub() ).clear();

    CAPTURE( target.c_str() );
    salvage_actor test_actor;
    item cut_up_target( target );
    item tool( itype_knife_huge );
    const std::map<material_id, int> &target_materials = cut_up_target.made_of();
    const float mat_total = cut_up_target.type->mat_portion_total == 0 ? 1 :
                            cut_up_target.type->mat_portion_total;
    units::mass smallest_yield_mass = units::mass::max();
    for( const auto &mater : target_materials ) {
        if( const std::optional<itype_id> item_id = mater.first->salvaged_into() ) {
            units::mass portioned_weight = item_id->obj().weight * ( mater.second / mat_total );
            smallest_yield_mass = std::min( smallest_yield_mass, portioned_weight );
        }
    }
    REQUIRE( smallest_yield_mass != units::mass::max() );

    units::mass cut_up_target_mass = cut_up_target.weight();
    item &spawned_item = here.add_item_or_charges( guy.pos_bub(), cut_up_target );
    item_location item_loc( map_cursor( guy.pos_abs() ), &spawned_item );

    REQUIRE( smallest_yield_mass <= cut_up_target_mass );

    test_actor.try_to_cut_up( guy, tool, item_loc );

    map_stack salvaged_items = here.i_at( guy.pos_bub() );
    units::mass salvaged_mass = 0_gram;
    for( const item &salvage : salvaged_items ) {
        salvaged_mass += salvage.weight();
    }
    CHECK( salvaged_mass <= cut_up_target_mass );
}

TEST_CASE( "cut_up_yields" )
{
    cut_up_yields( itype_blanket );
    cut_up_yields( itype_backpack_hiking );
    cut_up_yields( itype_boxpack );
    cut_up_yields( itype_case_violin );
    cut_up_yields( itype_down_mattress );
    cut_up_yields( itype_plastic_boat_hull );
    cut_up_yields( itype_bunker_coat );
    cut_up_yields( itype_bunker_pants );
    cut_up_yields( itype_kevlar_harness );
    cut_up_yields( itype_touring_suit );
    cut_up_yields( itype_dress_wedding );
    cut_up_yields( itype_wetsuit );
    cut_up_yields( itype_wetsuit_booties );
    cut_up_yields( itype_wetsuit_hood );
    cut_up_yields( itype_wetsuit_spring );
    cut_up_yields( itype_wetsuit_gloves );
    cut_up_yields( itype_peacoat );
    cut_up_yields( itype_log );
    cut_up_yields( itype_stick );
    cut_up_yields( itype_stick_long );
    cut_up_yields( itype_tazer );
    cut_up_yields( itype_laptop );
    cut_up_yields( itype_voltmeter );
    cut_up_yields( itype_burette );
    cut_up_yields( itype_eink_tablet_pc );
    cut_up_yields( itype_camera );
    cut_up_yields( itype_cell_phone );
    cut_up_yields( itype_laptop );
    cut_up_yields( itype_radio );
    cut_up_yields( itype_under_armor );
    cut_up_yields( itype_acidchitin_harness_dog );
    cut_up_yields( itype_chitin_harness_dog );
    cut_up_yields( itype_leather_harness_dog );
    cut_up_yields( itype_leatherbone_harness_dog );
    cut_up_yields( itype_kevlar_harness );
    cut_up_yields( itype_rubber_harness_dog );
}
