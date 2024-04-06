#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "avatar.h"
#include "bionics.h"
#include "cata_catch.h"
#include "character.h"
#include "character_id.h"
#include "character_martial_arts.h"
#include "game.h"
#include "inventory.h"
#include "item.h"
#include "itype.h"
#include "make_static.h"
#include "map.h"
#include "npc.h"
#include "pimpl.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "profession.h"
#include "ret_val.h"
#include "skill.h"
#include "stomach.h"
#include "type_id.h"

static const itype_id itype_debug_backpack( "debug_backpack" );

static const move_mode_id move_mode_walk( "walk" );

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

// Return true if character has an item with get_var( var ) set to the given value
bool character_has_item_with_var_val( const Character &they, const std::string &var,
                                      const std::string &val )
{
    return they.has_item_with( [var, val]( const item & cand ) {
        return cand.get_var( var ) == val;
    } );
}

void clear_character( Character &dummy, bool skip_nutrition )
{
    dummy.set_body();
    dummy.normalize(); // In particular this clears martial arts style

    // delete all worn items.
    dummy.clear_worn();
    dummy.calc_encumbrance();
    dummy.invalidate_crafting_inventory();
    dummy.inv->clear();
    dummy.remove_weapon();
    dummy.clear_mutations();
    dummy.mutation_category_level.clear();
    dummy.clear_bionics();

    // Clear stomach and then eat a nutritious meal to normalize stomach
    // contents (needs to happen before clear_morale).
    dummy.stomach.empty();
    dummy.guts.empty();
    dummy.clear_vitamins();
    dummy.health_tally = 0;
    dummy.update_body( calendar::turn, calendar::turn ); // sets last_updated to current turn
    if( !skip_nutrition ) {
        item food( "debug_nutrition" );
        dummy.consume( food );
    }

    // This sets HP to max, clears addictions and morale,
    // and sets hunger, thirst, sleepiness and such to zero
    dummy.environmental_revert_effect();
    // However, the above does not set stored kcal
    dummy.set_stored_kcal( dummy.get_healthy_kcal() );

    dummy.prof = profession::generic();
    dummy.hobbies.clear();
    dummy._skills->clear();
    dummy.martial_arts_data->clear_styles();
    dummy.clear_morale();
    dummy.activity.set_to_null();
    dummy.backlog.clear();
    dummy.reset_chargen_attributes();
    dummy.set_pain( 0 );
    dummy.reset_bonuses();
    dummy.set_speed_base( 100 );
    dummy.set_speed_bonus( 0 );
    dummy.set_sleep_deprivation( 0 );
    dummy.set_moves( 0 );
    dummy.oxygen = dummy.get_oxygen_max();
    for( const proficiency_id &prof : dummy.known_proficiencies() ) {
        dummy.lose_proficiency( prof, true );
    }
    for( const proficiency_id &prof : dummy.learning_proficiencies() ) {
        dummy.set_proficiency_practiced_time( prof, 0 );
    }

    // Reset cardio_acc to baseline
    dummy.reset_cardio_acc();
    // Restore all stamina and go to walk mode
    dummy.set_stamina( dummy.get_stamina_max() );
    dummy.set_movement_mode( move_mode_walk );
    dummy.reset_activity_level();

    // Make sure we don't carry around weird effects.
    dummy.clear_effects();
    dummy.set_underwater( false );

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
    dummy.clear_values();
    dummy.magic = pimpl<known_magic>();
    dummy.forget_all_recipes();
    dummy.set_focus( dummy.calc_focus_equilibrium() );
}

void arm_shooter( Character &shooter, const std::string &gun_type,
                  const std::vector<std::string> &mods,
                  const std::string &ammo_type )
{
    shooter.remove_weapon();
    // XL so arrows can fit.
    if( !shooter.is_wearing( itype_debug_backpack ) ) {
        shooter.worn.wear_item( shooter, item( "debug_backpack" ), false, false );
    }

    const itype_id &gun_id{ itype_id( gun_type ) };
    // Give shooter a loaded gun of the requested type.
    item_location gun = shooter.i_add( item( gun_id ) );
    itype_id ammo_id;
    // if ammo is not supplied we want the default
    if( ammo_type.empty() ) {
        if( gun->ammo_default().is_null() ) {
            ammo_id = item( gun->magazine_default() ).ammo_default();
        } else {
            ammo_id = gun->ammo_default();
        }
    } else {
        ammo_id = itype_id( ammo_type );
    }
    const ammotype &type_of_ammo = item::find_type( ammo_id )->ammo->type;
    if( gun->magazine_integral() ) {
        item_location ammo = shooter.i_add( item( ammo_id, calendar::turn,
                                            gun->ammo_capacity( type_of_ammo ) ) );
        REQUIRE( gun->can_reload_with( *ammo, true ) );
        REQUIRE( shooter.can_reload( *gun, &*ammo ) );
        gun->reload( shooter, ammo, gun->ammo_capacity( type_of_ammo ) );
    } else {
        const itype_id magazine_id = gun->magazine_default();
        item_location magazine = shooter.i_add( item( magazine_id ) );
        item_location ammo = shooter.i_add( item( ammo_id, calendar::turn,
                                            magazine->ammo_capacity( type_of_ammo ) ) );
        REQUIRE( magazine->can_reload_with( *ammo,  true ) );
        REQUIRE( shooter.can_reload( *magazine, &*ammo ) );
        magazine->reload( shooter, ammo, magazine->ammo_capacity( type_of_ammo ) );
        gun->reload( shooter, magazine, magazine->ammo_capacity( type_of_ammo ) );
    }
    for( const std::string &mod : mods ) {
        gun->put_in( item( itype_id( mod ) ), pocket_type::MOD );
    }
    shooter.wield( *gun );
}

void clear_avatar()
{
    avatar &avatar = get_avatar();
    clear_character( avatar );
    avatar.clear_identified();
    avatar.clear_nutrition();
    avatar.reset_all_missions();
}

void equip_shooter( npc &shooter, const std::vector<std::string> &apparel )
{
    CHECK( !shooter.in_vehicle );
    shooter.clear_worn();
    shooter.inv->clear();
    for( const std::string &article : apparel ) {
        shooter.wear_item( item( article ) );
    }
}

void process_activity( Character &dummy )
{
    do {
        dummy.mod_moves( dummy.get_speed() );
        while( dummy.get_moves() > 0 && dummy.activity ) {
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

// Clear player traits and give them a single trait by name
void set_single_trait( Character &dummy, const std::string &trait_name )
{
    dummy.clear_mutations();
    dummy.toggle_trait( trait_id( trait_name ) );
    REQUIRE( dummy.has_trait( trait_id( trait_name ) ) );
}

void give_and_activate_bionic( Character &you, bionic_id const &bioid )
{
    INFO( "bionic " + bioid.str() + " is valid" );
    REQUIRE( bioid.is_valid() );

    you.add_bionic( bioid );
    INFO( "dummy has gotten " + bioid.str() + " bionic " );
    REQUIRE( you.has_bionic( bioid ) );

    // get bionic's index - might not be "last added" due to "integrated" ones
    int bioindex = -1;
    for( size_t i = 0; i < you.my_bionics->size(); i++ ) {
        const bionic &bio = ( *you.my_bionics )[ i ];
        if( bio.id == bioid ) {
            bioindex = i;
        }
    }
    REQUIRE( bioindex != -1 );

    bionic &bio = you.bionic_at_index( bioindex );
    REQUIRE( bio.id == bioid );

    // turn on if possible
    if( bio.id->has_flag( STATIC( json_character_flag( "BIONIC_TOGGLED" ) ) ) && !bio.powered ) {
        const std::vector<material_id> fuel_opts = bio.info().fuel_opts;
        if( !fuel_opts.empty() ) {
            you.set_value( fuel_opts.front().str(), "2" );
        }
        you.activate_bionic( bio );
        INFO( "bionic " + bio.id.str() + " with index " + std::to_string( bioindex ) + " is active " );
        REQUIRE( you.has_active_bionic( bioid ) );
        if( !fuel_opts.empty() ) {
            you.remove_value( fuel_opts.front().str() );
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
        tool_it.put_in( tool_it_mag, pocket_type::MAGAZINE_WELL );
    }
    return tool_it;
}
