#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "avatar.h"
#include "bionics.h"
#include "catch/catch.hpp"
#include "character.h"
#include "character_id.h"
#include "character_martial_arts.h"
#include "game.h"
#include "inventory.h"
#include "item.h"
#include "item_pocket.h"
#include "itype.h"
#include "make_static.h"
#include "map.h"
#include "npc.h"
#include "pimpl.h"
#include "player.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "point.h"
#include "ret_val.h"
#include "stomach.h"
#include "type_id.h"

int get_remaining_charges( const std::string &tool_id )
{
    const inventory crafting_inv = get_player_character().crafting_inventory();
    std::vector<const item *> items =
    crafting_inv.items_with( [tool_id]( const item & i ) {
        return i.typeId() == itype_id( tool_id );
    } );
    int remaining_charges = 0;
    for( const item *instance : items ) {
        remaining_charges += instance->ammo_remaining();
    }
    return remaining_charges;
}

bool player_has_item_of_type( const std::string &type )
{
    std::vector<item *> matching_items = get_player_character().inv->items_with(
    [&]( const item & i ) {
        return i.type->get_id() == itype_id( type );
    } );

    return !matching_items.empty();
}

void clear_character( player &dummy )
{
    dummy.set_body();
    dummy.normalize(); // In particular this clears martial arts style

    // delete all worn items.
    dummy.worn.clear();
    dummy.calc_encumbrance();
    dummy.inv->clear();
    dummy.remove_weapon();
    dummy.clear_mutations();

    // Clear stomach and then eat a nutritious meal to normalize stomach
    // contents (needs to happen before clear_morale).
    dummy.stomach.empty();
    dummy.guts.empty();
    item food( "debug_nutrition" );
    dummy.consume( food );

    // This sets HP to max, clears addictions and morale,
    // and sets hunger, thirst, fatigue and such to zero
    dummy.environmental_revert_effect();
    // However, the above does not set stored kcal
    dummy.set_stored_kcal( dummy.get_healthy_kcal() );

    dummy.empty_skills();
    dummy.martial_arts_data->clear_styles();
    dummy.clear_morale();
    dummy.clear_bionics();
    dummy.activity.set_to_null();
    dummy.reset_chargen_attributes();
    dummy.set_pain( 0 );
    dummy.reset_bonuses();
    dummy.set_speed_base( 100 );
    dummy.set_speed_bonus( 0 );
    dummy.set_sleep_deprivation( 0 );
    for( const proficiency_id &prof : dummy.known_proficiencies() ) {
        dummy.lose_proficiency( prof, true );
    }

    // Restore all stamina and go to walk mode
    dummy.set_stamina( dummy.get_stamina_max() );
    dummy.set_movement_mode( move_mode_id( "walk" ) );
    dummy.reset_activity_level();

    // Make sure we don't carry around weird effects.
    dummy.clear_effects();

    // Make stats nominal.
    dummy.str_max = 8;
    dummy.dex_max = 8;
    dummy.int_max = 8;
    dummy.per_max = 8;
    dummy.set_str_bonus( 0 );
    dummy.set_dex_bonus( 0 );
    dummy.set_int_bonus( 0 );
    dummy.set_per_bonus( 0 );

    dummy.cash = 0;

    const tripoint spot( 60, 60, 0 );
    dummy.setpos( spot );
}

void arm_shooter( npc &shooter, const std::string &gun_type,
                  const std::vector<std::string> &mods,
                  const std::string &ammo_type )
{
    shooter.remove_weapon();
    // XL so arrows can fit.
    if( !shooter.is_wearing( itype_id( "debug_backpack" ) ) ) {
        shooter.worn.push_back( item( "debug_backpack" ) );
    }

    const itype_id &gun_id{ itype_id( gun_type ) };
    // Give shooter a loaded gun of the requested type.
    item &gun = shooter.i_add( item( gun_id ) );
    itype_id ammo_id;
    // if ammo is not supplied we want the default
    if( ammo_type.empty() ) {
        if( gun.ammo_default().is_null() ) {
            ammo_id = item( gun.magazine_default() ).ammo_default();
        } else {
            ammo_id = gun.ammo_default();
        }
    } else {
        ammo_id = itype_id( ammo_type );
    }
    const ammotype &type_of_ammo = item::find_type( ammo_id )->ammo->type;
    if( gun.magazine_integral() ) {
        item &ammo = shooter.i_add( item( ammo_id, calendar::turn, gun.ammo_capacity( type_of_ammo ) ) );
        REQUIRE( gun.is_reloadable_with( ammo_id ) );
        REQUIRE( shooter.can_reload( gun, ammo_id ) );
        gun.reload( shooter, item_location( shooter, &ammo ), gun.ammo_capacity( type_of_ammo ) );
    } else {
        const itype_id magazine_id = gun.magazine_default();
        item &magazine = shooter.i_add( item( magazine_id ) );
        item &ammo = shooter.i_add( item( ammo_id, calendar::turn,
                                          magazine.ammo_capacity( type_of_ammo ) ) );
        REQUIRE( magazine.is_reloadable_with( ammo_id ) );
        REQUIRE( shooter.can_reload( magazine, ammo_id ) );
        magazine.reload( shooter, item_location( shooter, &ammo ), magazine.ammo_capacity( type_of_ammo ) );
        gun.reload( shooter, item_location( shooter, &magazine ), magazine.ammo_capacity( type_of_ammo ) );
    }
    for( const auto &mod : mods ) {
        gun.put_in( item( itype_id( mod ) ), item_pocket::pocket_type::MOD );
    }
    shooter.wield( gun );
}

void clear_avatar()
{
    clear_character( get_avatar() );
    get_avatar().clear_identified();
}

void process_activity( player &dummy )
{
    do {
        dummy.moves += dummy.get_speed();
        while( dummy.moves > 0 && dummy.activity ) {
            dummy.activity.do_turn( dummy );
        }
    } while( dummy.activity );
}

npc &spawn_npc( const point &p, const std::string &npc_class )
{
    const string_id<npc_template> test_guy( npc_class );
    const character_id model_id = get_map().place_npc( p, test_guy );
    g->load_npcs();

    npc *guy = g->find_npc( model_id );
    REQUIRE( guy != nullptr );
    CHECK( !guy->in_vehicle );
    return *guy;
}

void give_and_activate_bionic( player &p, bionic_id const &bioid )
{
    INFO( "bionic " + bioid.str() + " is valid" );
    REQUIRE( bioid.is_valid() );

    p.add_bionic( bioid );
    INFO( "dummy has gotten " + bioid.str() + " bionic " );
    REQUIRE( p.has_bionic( bioid ) );

    // get bionic's index - might not be "last added" due to "integrated" ones
    int bioindex = -1;
    for( size_t i = 0; i < p.my_bionics->size(); i++ ) {
        const auto &bio = ( *p.my_bionics )[ i ];
        if( bio.id == bioid ) {
            bioindex = i;
        }
    }
    REQUIRE( bioindex != -1 );

    const bionic &bio = p.bionic_at_index( bioindex );
    REQUIRE( bio.id == bioid );

    // turn on if possible
    if( bio.id->has_flag( STATIC( json_character_flag( "BIONIC_TOGGLED" ) ) ) && !bio.powered ) {
        const std::vector<material_id> fuel_opts = bio.info().fuel_opts;
        if( !fuel_opts.empty() ) {
            p.set_value( fuel_opts.front().str(), "2" );
        }
        p.activate_bionic( bioindex );
        INFO( "bionic " + bio.id.str() + " with index " + std::to_string( bioindex ) + " is active " );
        REQUIRE( p.has_active_bionic( bioid ) );
        if( !fuel_opts.empty() ) {
            p.remove_value( fuel_opts.front().str() );
        }
    }
}

item tool_with_ammo( const std::string &tool, const int qty )
{
    item tool_it( tool );
    if( !tool_it.ammo_default().is_null() ) {
        tool_it.ammo_set( tool_it.ammo_default(), qty );
    } else if( !tool_it.magazine_default().is_null() ) {
        item tool_it_mag( tool_it.magazine_default() );
        tool_it_mag.ammo_set( tool_it_mag.ammo_default(), qty );
        tool_it.put_in( tool_it_mag, item_pocket::pocket_type::MAGAZINE_WELL );
    }
    return tool_it;
}
