#include "monexamine.h"

#include <functional>
#include <iosfwd>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "activity_actor_definitions.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "creature.h"
#include "debug.h"
#include "enums.h"
#include "game.h"
#include "game_inventory.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "iuse.h"
#include "map.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "output.h"
#include "player_activity.h"
#include "point.h"
#include "rng.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "type_id.h"
#include "ui.h"
#include "units.h"
#include "value_ptr.h"

static const efftype_id effect_controlled( "controlled" );
static const efftype_id effect_harnessed( "harnessed" );
static const efftype_id effect_has_bag( "has_bag" );
static const efftype_id effect_leashed( "leashed" );
static const efftype_id effect_led_by_leash( "led_by_leash" );
static const efftype_id effect_monster_armor( "monster_armor" );
static const efftype_id effect_monster_saddled( "monster_saddled" );
static const efftype_id effect_paid( "paid" );
static const efftype_id effect_pet( "pet" );
static const efftype_id effect_ridden( "ridden" );
static const efftype_id effect_sheared( "sheared" );
static const efftype_id effect_tied( "tied" );

static const flag_id json_flag_MECH_BAT( "MECH_BAT" );
static const flag_id json_flag_TACK( "TACK" );
static const flag_id json_flag_TIE_UP( "TIE_UP" );

static const itype_id itype_cash_card( "cash_card" );
static const itype_id itype_id_military( "id_military" );

static const quality_id qual_CUT( "CUT" );
static const quality_id qual_SHEAR( "SHEAR" );

static const skill_id skill_survival( "survival" );

namespace
{

item_location tack_loc()
{
    auto filter = []( const item & it ) {
        return it.has_flag( json_flag_TACK );
    };

    return game_menus::inv::titled_filter_menu( filter, get_avatar(), _( "Tack" ) );
}

void attach_saddle_to( monster &z )
{
    if( z.has_effect( effect_monster_saddled ) ) {
        return;
    }
    item_location loc = tack_loc();
    if( !loc ) {
        add_msg( _( "Never mind." ) );
        return;
    }
    z.add_effect( effect_monster_saddled, 1_turns, true );
    z.tack_item = cata::make_value<item>( *loc.get_item() );
    loc.remove_item();
}

void remove_saddle_from( monster &z )
{
    if( !z.has_effect( effect_monster_saddled ) ) {
        return;
    }
    z.remove_effect( effect_monster_saddled );
    get_player_character().i_add( *z.tack_item );
    z.tack_item.reset();
}

void mount_pet( monster &z )
{
    get_player_character().mount_creature( z );
}

void swap( monster &z )
{
    std::string pet_name = z.get_name();
    Character &player_character = get_player_character();
    player_character.moves -= 150;

    ///\EFFECT_STR increases chance to successfully swap positions with your pet
    ///\EFFECT_DEX increases chance to successfully swap positions with your pet
    if( !one_in( ( player_character.str_cur + player_character.dex_cur ) / 6 ) ) {
        bool t = z.has_effect( effect_tied );
        if( t ) {
            z.remove_effect( effect_tied );
        }

        g->swap_critters( player_character, z );

        if( t ) {
            z.add_effect( effect_tied, 1_turns, true );
        }
        add_msg( _( "You swap positions with your %s." ), pet_name );
    } else {
        add_msg( _( "You fail to budge your %s!" ), pet_name );
    }
}

void push( monster &z )
{
    std::string pet_name = z.get_name();
    Character &player_character = get_player_character();
    player_character.moves -= 30;

    ///\EFFECT_STR increases chance to successfully push your pet
    if( one_in( player_character.str_cur ) ) {
        add_msg( _( "You pushed the %s, but it resisted." ), pet_name );
        return;
    }

    point delta( z.posx() - player_character.posx(), z.posy() - player_character.posy() );
    if( z.move_to( tripoint( z.posx( ) + delta.x, z.posy( ) + delta.y, z.posz( ) ) ) ) {
        add_msg( _( "You pushed the %s." ), pet_name );
    } else {
        add_msg( _( "You pushed the %s, but it resisted." ), pet_name );
        return;
    }
}

void rename_pet( monster &z )
{
    std::string unique_name = string_input_popup()
                              .title( _( "Enter new pet name:" ) )
                              .width( 20 )
                              .query_string();
    if( !unique_name.empty() ) {
        z.unique_name = unique_name;
    }
}

void attach_bag_to( monster &z )
{
    std::string pet_name = z.get_name();

    auto filter = []( const item & it ) {
        return it.is_armor() && it.get_total_capacity() > 0_ml;
    };

    avatar &player_character = get_avatar();
    item_location loc = game_menus::inv::titled_filter_menu( filter, player_character,
                        _( "Bag item" ) );

    if( !loc ) {
        add_msg( _( "Never mind." ) );
        return;
    }

    item &it = *loc;
    z.storage_item = cata::make_value<item>( it );
    add_msg( _( "You mount the %1$s on your %2$s." ), it.display_name(), pet_name );
    player_character.i_rem( &it );
    z.add_effect( effect_has_bag, 1_turns, true );
    // Update encumbrance in case we were wearing it
    player_character.flag_encumbrance();
    player_character.moves -= 200;
}

void dump_items( monster &z )
{
    std::string pet_name = z.get_name();
    Character &player_character = get_player_character();
    map &here = get_map();
    for( item &it : z.inv ) {
        if( it.has_var( "DESTROY_ITEM_ON_MON_DEATH" ) ) {
            continue;
        }
        here.add_item_or_charges( player_character.pos(), it );
    }
    z.inv.clear();
    add_msg( _( "You dump the contents of the %s's bag on the ground." ), pet_name );
    player_character.moves -= 200;
}

void remove_bag_from( monster &z )
{
    std::string pet_name = z.get_name();
    if( z.storage_item ) {
        if( !z.inv.empty() ) {
            dump_items( z );
        }
        Character &player_character = get_player_character();
        get_map().add_item_or_charges( player_character.pos(), *z.storage_item );
        add_msg( _( "You remove the %1$s from %2$s." ), z.storage_item->display_name(), pet_name );
        z.storage_item.reset();
        player_character.moves -= 200;
    } else {
        add_msg( m_bad, _( "Your %1$s doesn't have a bag!" ), pet_name );
    }
    z.remove_effect( effect_has_bag );
}

bool give_items_to( monster &z )
{
    std::string pet_name = z.get_name();
    if( !z.storage_item ) {
        add_msg( _( "There is no container on your %s to put things in!" ), pet_name );
        return true;
    }

    item &storage = *z.storage_item;
    units::mass max_weight = z.weight_capacity() - z.get_carried_weight();
    units::volume max_volume = storage.get_total_capacity() - z.get_carried_volume();

    avatar &player_character = get_avatar();
    drop_locations items = game_menus::inv::multidrop( player_character );
    drop_locations to_move;
    for( const drop_location &itq : items ) {
        const item &it = *itq.first;
        units::volume item_volume = it.volume() * itq.second;
        units::mass item_weight = it.weight() * itq.second;
        if( max_weight < item_weight ) {
            add_msg( _( "The %1$s is too heavy for the %2$s to carry." ), it.tname(), pet_name );
            continue;
        } else if( max_volume < item_volume ) {
            add_msg( _( "The %1$s is too big to fit in the %2$s." ), it.tname(), storage.tname() );
            continue;
        } else {
            max_weight -= item_weight;
            max_volume -= item_volume;
            to_move.insert( to_move.end(), itq );
        }
    }
    // Quit if there is nothing to add
    if( to_move.empty() ) {
        add_msg( _( "Never mind." ) );
        return true;
    }
    z.add_effect( effect_controlled, 5_turns );
    player_character.drop( to_move, z.pos(), true );
    // Print an appropriate message for the inserted item or items
    if( to_move.size() > 1 ) {
        add_msg( _( "You put %1$s items in the %2$s on your %3$s." ), to_move.size(), storage.tname(),
                 pet_name );
    } else {
        item_location loc = to_move.front().first;
        item &it = *loc;
        //~ %1$s - item name, %2$s - storage item name, %3$s - pet name
        add_msg( _( "You put the %1$s in the %2$s on your %3$s." ), it.tname(), storage.tname(), pet_name );
    }
    // Return success if all items were inserted
    return to_move.size() == items.size();
}

item_location pet_armor_loc( monster &z )
{
    auto filter = [z]( const item & it ) {
        return z.type->bodytype == it.get_pet_armor_bodytype() &&
               z.get_volume() >= it.get_pet_armor_min_vol() &&
               z.get_volume() <= it.get_pet_armor_max_vol();
    };

    return game_menus::inv::titled_filter_menu( filter, get_avatar(), _( "Pet armor" ) );
}

bool add_armor( monster &z )
{
    std::string pet_name = z.get_name();
    item_location loc = pet_armor_loc( z );

    if( !loc ) {
        add_msg( _( "Never mind." ) );
        return true;
    }

    item &armor = *loc;
    units::mass max_weight = z.weight_capacity() - z.get_carried_weight();
    if( max_weight <= armor.weight() ) {
        add_msg( pgettext( "pet armor", "Your %1$s is too heavy for your %2$s." ), armor.tname( 1 ),
                 pet_name );
        return true;
    }

    armor.set_var( "pet_armor", "true" );
    z.armor_item = cata::make_value<item>( armor );
    add_msg( pgettext( "pet armor", "You put the %1$s on your %2$s." ), armor.display_name(),
             pet_name );
    loc.remove_item();
    z.add_effect( effect_monster_armor, 1_turns, true );
    // TODO: armoring a horse takes a lot longer than 2 seconds. This should be a long action.
    get_player_character().moves -= 200;
    return true;
}

void remove_harness( monster &z )
{
    z.remove_effect( effect_harnessed );
    add_msg( m_info, _( "You unhitch %s from the vehicle." ), z.get_name() );
}

void remove_armor( monster &z )
{
    std::string pet_name = z.get_name();
    if( z.armor_item ) {
        z.armor_item->erase_var( "pet_armor" );
        get_map().add_item_or_charges( z.pos(), *z.armor_item );
        add_msg( pgettext( "pet armor", "You remove the %1$s from %2$s." ), z.armor_item->display_name(),
                 pet_name );
        z.armor_item.reset();
        // TODO: removing armor from a horse takes a lot longer than 2 seconds. This should be a long action.
        get_player_character().moves -= 200;
    } else {
        add_msg( m_bad, _( "Your %1$s isn't wearing armor!" ), pet_name );
    }
    z.remove_effect( effect_monster_armor );
}

void play_with( monster &z )
{
    std::string pet_name = z.get_name();
    Character &player_character = get_player_character();
    const std::string &petstr = z.type->petfood.pet;
    player_character.assign_activity(
        player_activity( play_with_pet_activity_actor( pet_name, petstr ) ) );
}

void cull( monster &z )
{
    Character &player_character = get_player_character();
    if( !player_character.has_quality( qual_CUT ) ) {
        add_msg( _( "You don't have a cutting tool." ) );
        return;
    }
    z.apply_damage( nullptr, bodypart_id( "torso" ), z.get_hp() );
}

void add_leash( monster &z )
{
    if( z.has_effect( effect_leashed ) ) {
        return;
    }
    Character &player_character = get_player_character();
    std::vector<item *> rope_inv = player_character.items_with( []( const item & itm ) {
        return itm.has_flag( json_flag_TIE_UP );
    } );
    if( rope_inv.empty() ) {
        return;
    }
    int i = 0;
    uilist selection_menu;
    selection_menu.text = string_format( _( "Select an item to leash your %s with." ), z.get_name() );
    selection_menu.addentry( i++, true, MENU_AUTOASSIGN, _( "Cancel" ) );
    for( const item *iter : rope_inv ) {
        selection_menu.addentry( i++, true, MENU_AUTOASSIGN, _( "Use %s" ), iter->tname() );
    }
    selection_menu.selected = 1;
    selection_menu.query();
    int index = selection_menu.ret;
    if( index == 0 || index == UILIST_CANCEL || index < 0 ||
        index > static_cast<int>( rope_inv.size() ) ) {
        return;
    }
    item *rope_item = rope_inv[index - 1];
    z.tied_item = cata::make_value<item>( *rope_item );
    player_character.i_rem( rope_item );
    z.add_effect( effect_leashed, 1_turns, true );
    add_msg( _( "You add a leash to your %s." ), z.get_name() );
}

void remove_leash( monster &z )
{
    if( !z.has_effect( effect_leashed ) ) {
        return;
    }
    z.remove_effect( effect_led_by_leash );
    z.remove_effect( effect_leashed );
    if( z.tied_item ) {
        get_player_character().i_add( *z.tied_item );
        z.tied_item.reset();
    }
    add_msg( _( "You remove the leash from your %s." ), z.get_name() );
}

void tie_pet( monster &z )
{
    if( z.has_effect( effect_tied ) ) {
        return;
    }
    z.add_effect( effect_tied, 1_turns, true );
    add_msg( _( "You tie your %s." ), z.get_name() );
}

void untie_pet( monster &z )
{
    if( !z.has_effect( effect_tied ) ) {
        return;
    }
    z.remove_effect( effect_tied );
    if( !z.has_effect( effect_leashed ) ) {
        // migration code dealing with animals tied before leashing was introduced
        z.add_effect( effect_leashed, 1_turns, true );
    }
    add_msg( _( "You untie your %s." ), z.get_name() );
}

void start_leading( monster &z )
{
    if( z.has_effect( effect_led_by_leash ) ) {
        return;
    }
    if( z.has_effect( effect_tied ) ) {
        untie_pet( z );
    }
    z.add_effect( effect_led_by_leash, 1_turns, true );
    add_msg( _( "You take hold of the %s's leash to make it follow you." ), z.get_name() );
}

void stop_leading( monster &z )
{
    if( !z.has_effect( effect_led_by_leash ) ) {
        return;
    }
    z.remove_effect( effect_led_by_leash );
    // The pet may or may not stop following so don't print that here
    add_msg( _( "You release the %s's leash." ), z.get_name() );
}

/*
 * Manages the milking and milking cool down of monsters.
 * Milked item uses starting_ammo, where ammo type is the milked item
 * and amount the times per day you can milk the monster.
 */
void milk_source( monster &source_mon )
{
    itype_id milked_item = source_mon.type->starting_ammo.begin()->first;
    auto milkable_ammo = source_mon.ammo.find( milked_item );
    if( milkable_ammo == source_mon.ammo.end() ) {
        debugmsg( "The %s has no milkable %s.", source_mon.get_name(), milked_item.str() );
        return;
    }
    if( milkable_ammo->second > 0 ) {
        const int moves = to_moves<int>( time_duration::from_minutes( milkable_ammo->second / 2 ) );
        std::vector<tripoint> coords{};
        std::vector<std::string> str_values{};
        Character &player_character = get_player_character();
        coords.push_back( get_map().getabs( source_mon.pos() ) );
        // pin the cow in place if it isn't already
        bool temp_tie = !source_mon.has_effect( effect_tied );
        if( temp_tie ) {
            source_mon.add_effect( effect_tied, 1_turns, true );
            str_values.emplace_back( "temp_tie" );
        }
        player_character.assign_activity( player_activity( milk_activity_actor( moves, coords,
                                          str_values ) ) );

        add_msg( _( "You milk the %s." ), source_mon.get_name() );
    } else {
        add_msg( _( "The %s has no more milk." ), source_mon.get_name() );
    }
}

void shear_animal( monster &z )
{
    Character &guy = get_player_character();
    if( !guy.has_quality( qual_SHEAR ) ) {
        add_msg( _( "You don't have a shearing tool." ) );
    }

    // was monster already tied before shearing
    const bool monster_tied = z.has_effect( effect_tied );

    // tie the critter so it doesn't move while being sheared
    if( !monster_tied ) {
        z.add_effect( effect_tied, 1_turns, true );
    }

    guy.assign_activity( player_activity( shearing_activity_actor( z.pos(), !monster_tied ) ) );
}

void remove_battery( monster &z )
{
    get_map().add_item_or_charges( get_player_character().pos(), *z.battery_item );
    z.battery_item.reset();
}

void insert_battery( monster &z )
{
    if( z.battery_item ) {
        // already has a battery, shouldn't be called with one, but just in case.
        return;
    }
    Character &player_character = get_player_character();
    std::vector<item *> bat_inv = player_character.items_with( []( const item & itm ) {
        return itm.has_flag( json_flag_MECH_BAT );
    } );
    if( bat_inv.empty() ) {
        return;
    }
    int i = 0;
    uilist selection_menu;
    selection_menu.text = string_format( _( "Select an battery to insert into your %s." ),
                                         z.get_name() );
    selection_menu.addentry( i++, true, MENU_AUTOASSIGN, _( "Cancel" ) );
    for( const item *iter : bat_inv ) {
        selection_menu.addentry( i++, true, MENU_AUTOASSIGN, _( "Use %s" ), iter->tname() );
    }
    selection_menu.selected = 1;
    selection_menu.query();
    int index = selection_menu.ret;
    if( index == 0 || index == UILIST_CANCEL || index < 0 ||
        index > static_cast<int>( bat_inv.size() ) ) {
        return;
    }
    item *bat_item = bat_inv[index - 1];
    z.battery_item = cata::make_value<item>( *bat_item );
    player_character.i_rem( bat_item );
}

} // namespace

bool Character::can_mount( const monster &critter ) const
{
    const auto &avoid = get_path_avoid();
    auto route = get_map().route( pos(), critter.pos(), get_pathfinding_settings(), avoid );

    if( route.empty() ) {
        return false;
    }
    return ( critter.has_flag( MF_PET_MOUNTABLE ) && critter.friendly == -1 &&
             !critter.has_effect( effect_controlled ) && !critter.has_effect( effect_ridden ) ) &&
           ( ( critter.has_effect( effect_monster_saddled ) && get_skill_level( skill_survival ) >= 1 ) ||
             get_skill_level( skill_survival ) >= 4 ) && ( critter.get_size() >= ( get_size() + 1 ) &&
                     get_weight() <= critter.get_weight() * critter.get_mountable_weight_ratio() );
}

bool monexamine::pet_menu( monster &z )
{
    enum choices {
        swap_pos = 0,
        push_monster,
        lead,
        stop_lead,
        rename,
        attach_bag,
        remove_bag,
        drop_all,
        give_items,
        mon_armor_add,
        mon_harness_remove,
        mon_armor_remove,
        leash,
        unleash,
        play_with_pet,
        cull_pet,
        milk,
        shear,
        pay,
        attach_saddle,
        remove_saddle,
        mount,
        tie,
        untie,
        remove_bat,
        insert_bat,
        check_bat,
        attack,
        talk_to
    };

    uilist amenu;
    std::string pet_name = z.get_name();

    amenu.text = string_format( _( "What to do with your %s?" ), pet_name );

    amenu.addentry( swap_pos, true, 's', _( "Swap positions" ) );
    amenu.addentry( push_monster, true, 'p', _( "Push %s" ), pet_name );
    if( z.has_effect( effect_leashed ) ) {
        if( z.has_effect( effect_led_by_leash ) ) {
            amenu.addentry( stop_lead, true, 'l', _( "Stop leading %s" ), pet_name );
        } else {
            amenu.addentry( lead, true, 'l', _( "Lead %s by the leash" ), pet_name );
        }
    }
    amenu.addentry( rename, true, 'e', _( "Rename" ) );
    amenu.addentry( attack, true, 'A', _( "Attack" ) );
    Character &player_character = get_player_character();
    if( z.has_effect( effect_has_bag ) ) {
        amenu.addentry( give_items, true, 'g', _( "Place items into bag" ) );
        amenu.addentry( remove_bag, true, 'b', _( "Remove bag from %s" ), pet_name );
        if( !z.inv.empty() ) {
            amenu.addentry( drop_all, true, 'd', _( "Remove all items from bag" ) );
        }
    } else if( !z.has_flag( MF_RIDEABLE_MECH ) ) {
        amenu.addentry( attach_bag, true, 'b', _( "Attach bag to %s" ), pet_name );
    }
    if( z.has_effect( effect_harnessed ) ) {
        amenu.addentry( mon_harness_remove, true, 'H', _( "Remove vehicle harness from %s" ), pet_name );
    }
    if( z.has_effect( effect_monster_armor ) ) {
        amenu.addentry( mon_armor_remove, true, 'a', _( "Remove armor from %s" ), pet_name );
    } else if( !z.has_flag( MF_RIDEABLE_MECH ) ) {
        amenu.addentry( mon_armor_add, true, 'a', _( "Equip %s with armor" ), pet_name );
    }
    if( z.has_effect( effect_tied ) ) {
        amenu.addentry( untie, true, 't', _( "Untie" ) );
    }
    if( z.has_effect( effect_leashed ) && !z.has_effect( effect_tied ) ) {
        amenu.addentry( tie, true, 't', _( "Tie" ) );
        amenu.addentry( unleash, true, 'L', _( "Remove leash from %s" ), pet_name );
    }
    if( !z.has_effect( effect_leashed ) && !z.has_flag( MF_RIDEABLE_MECH ) ) {
        std::vector<item *> rope_inv = player_character.items_with( []( const item & itm ) {
            return itm.has_flag( json_flag_TIE_UP );
        } );
        if( !rope_inv.empty() ) {
            amenu.addentry( leash, true, 't', _( "Attach leash to %s" ), pet_name );
        } else {
            amenu.addentry( leash, false, 't', _( "You need any type of rope to leash %s" ),
                            pet_name );
        }
    }

    if( z.has_flag( MF_CANPLAY ) ) {
        amenu.addentry( play_with_pet, true, 'y', _( "Play with %s" ), pet_name );
    }
    if( z.has_flag( MF_CAN_BE_CULLED ) ) {
        amenu.addentry( cull_pet, true, 'y', _( "Cull %s" ), pet_name );
    }
    if( z.has_flag( MF_MILKABLE ) ) {
        amenu.addentry( milk, true, 'm', _( "Milk %s" ), pet_name );
    }
    if( z.shearable() ) {
        bool available = true;
        if( season_of_year( calendar::turn ) == WINTER ) {
            amenu.addentry( shear, false, 'S',
                            _( "This animal would freeze if you shear it during winter." ) );
            available = false;
        } else if( z.has_effect( effect_sheared ) ) {
            amenu.addentry( shear, false, 'S', _( "This animal is not ready to be sheared again yet." ) );
            available = false;
        }
        if( available ) {
            if( player_character.has_quality( qual_SHEAR, 1 ) ) {
                amenu.addentry( shear, true, 'S', _( "Shear %s." ), pet_name );
            } else {
                amenu.addentry( shear, false, 'S', _( "You cannot shear this animal without a shearing tool." ) );
            }
        }
    }
    if( z.has_flag( MF_PET_MOUNTABLE ) && !z.has_effect( effect_monster_saddled ) &&
        player_character.has_item_with_flag( json_flag_TACK ) ) {
        if( player_character.get_skill_level( skill_survival ) >= 1 ) {
            amenu.addentry( attach_saddle, true, 'h', _( "Tack up %s" ), pet_name );
        } else {
            amenu.addentry( attach_saddle, false, 'h', _( "You don't know how to saddle %s" ), pet_name );
        }
    }
    if( z.has_flag( MF_PET_MOUNTABLE ) && z.has_effect( effect_monster_saddled ) ) {
        amenu.addentry( remove_saddle, true, 'h', _( "Remove tack from %s" ), pet_name );
    }
    if( z.has_flag( MF_PAY_BOT ) ) {
        amenu.addentry( pay, true, 'f', _( "Manage your friendship with %s" ), pet_name );
    }
    if( !z.type->chat_topics.empty() ) {
        amenu.addentry( talk_to, true, 'c', _( "Talk to %s" ), pet_name );
    }
    if( !z.has_flag( MF_RIDEABLE_MECH ) ) {
        if( z.has_flag( MF_PET_MOUNTABLE ) && player_character.can_mount( z ) ) {
            amenu.addentry( mount, true, 'r', _( "Mount %s" ), pet_name );
        } else if( !z.has_flag( MF_PET_MOUNTABLE ) ) {
            amenu.addentry( mount, false, 'r', _( "%s cannot be mounted" ), pet_name );
        } else if( z.get_size() <= player_character.get_size() ) {
            amenu.addentry( mount, false, 'r', _( "%s is too small to carry your weight" ), pet_name );
        } else if( player_character.get_skill_level( skill_survival ) < 1 ) {
            amenu.addentry( mount, false, 'r', _( "You have no knowledge of riding at all" ) );
        } else if( player_character.get_weight() >= z.get_weight() * z.get_mountable_weight_ratio() ) {
            amenu.addentry( mount, false, 'r', _( "You are too heavy to mount %s" ), pet_name );
        } else if( !z.has_effect( effect_monster_saddled ) &&
                   player_character.get_skill_level( skill_survival ) < 4 ) {
            amenu.addentry( mount, false, 'r', _( "You are not skilled enough to ride without a saddle" ) );
        }
    } else {
        const itype &type = *item::find_type( z.type->mech_battery );
        int max_charge = type.magazine->capacity;
        float charge_percent;
        if( z.battery_item ) {
            charge_percent = static_cast<float>( z.battery_item->ammo_remaining() ) / max_charge * 100;
        } else {
            charge_percent = 0.0;
        }
        amenu.addentry( check_bat, false, 'c', _( "%s battery level is %d%%" ), z.get_name(),
                        static_cast<int>( charge_percent ) );
        if( !player_character.get_wielded_item() && z.battery_item ) {
            amenu.addentry( mount, true, 'r', _( "Climb into the mech and take control" ) );
        } else if( player_character.get_wielded_item() ) {
            amenu.addentry( mount, false, 'r', _( "You cannot pilot the mech whilst wielding something" ) );
        } else if( !z.battery_item ) {
            amenu.addentry( mount, false, 'r', _( "This mech has a dead battery and won't turn on" ) );
        }
        if( z.battery_item ) {
            amenu.addentry( remove_bat, true, 'x', _( "Remove the mech's battery pack" ) );
        } else if( player_character.has_amount( z.type->mech_battery, 1 ) ) {
            amenu.addentry( insert_bat, true, 'x', _( "Insert a new battery pack" ) );
        } else {
            amenu.addentry( insert_bat, false, 'x', _( "You need a %s to power this mech" ), type.nname( 1 ) );
        }
    }
    amenu.query();
    int choice = amenu.ret;

    switch( choice ) {
        case swap_pos:
            swap( z );
            break;
        case push_monster:
            push( z );
            break;
        case lead:
            start_leading( z );
            break;
        case stop_lead:
            stop_leading( z );
            break;
        case rename:
            rename_pet( z );
            break;
        case attach_bag:
            attach_bag_to( z );
            break;
        case remove_bag:
            remove_bag_from( z );
            break;
        case drop_all:
            dump_items( z );
            break;
        case give_items:
            return give_items_to( z );
        case mon_armor_add:
            return add_armor( z );
        case mon_harness_remove:
            remove_harness( z );
            break;
        case mon_armor_remove:
            remove_armor( z );
            break;
        case play_with_pet:
            if( query_yn( _( "Spend a few minutes to play with your %s?" ), pet_name ) ) {
                play_with( z );
            }
            break;
        case cull_pet:
            if( query_yn( _( "Really slaughter your %s?" ), pet_name ) ) {
                cull( z );
            }
            break;
        case leash:
            add_leash( z );
            break;
        case unleash:
            remove_leash( z );
            break;
        case tie:
            tie_pet( z );
            break;
        case untie:
            untie_pet( z );
            break;
        case attach_saddle:
            attach_saddle_to( z );
            break;
        case remove_saddle:
            remove_saddle_from( z );
            break;
        case mount:
            mount_pet( z );
            break;
        case milk:
            milk_source( z );
            break;
        case shear:
            shear_animal( z );
            break;
        case pay:
            pay_bot( z );
            break;
        case remove_bat:
            remove_battery( z );
            break;
        case insert_bat:
            insert_battery( z );
            break;
        case check_bat:
            break;
        case attack:
            if( query_yn( _( "You may be attacked!  Proceed?" ) ) ) {
                get_player_character().melee_attack( z, true );
            }
            break;
        case talk_to:
            get_avatar().talk_to( get_talker_for( z ) );
            break;
        default:
            break;
    }
    return true;
}

bool monexamine::mech_hack( monster &z )
{
    Character &player_character = get_player_character();
    itype_id card_type = itype_id_military;
    if( player_character.has_amount( card_type, 1 ) ) {
        if( query_yn( _( "Swipe your ID card into the mech's security port?" ) ) ) {
            player_character.mod_moves( -100 );
            z.add_effect( effect_pet, 1_turns, true );
            z.friendly = -1;
            add_msg( m_good, _( "The %s whirs into life and opens its restraints to accept a pilot." ),
                     z.get_name() );
            player_character.use_amount( card_type, 1 );
            return true;
        }
    } else {
        add_msg( m_info, _( "You do not have the required ID card to activate this mech." ) );
    }
    return false;
}

static int prompt_for_amount( const char *const msg, const int max )
{
    const std::string formatted = string_format( msg, max );
    const int amount = string_input_popup()
                       .title( formatted )
                       .width( 20 )
                       .text( std::to_string( max ) )
                       .only_digits( true )
                       .query_int();

    return clamp( amount, 0, max );
}

bool monexamine::pay_bot( monster &z )
{
    Character &player_character = get_player_character();
    time_duration friend_time = z.get_effect_dur( effect_pet );
    const int charge_count = player_character.charges_of( itype_cash_card );

    int amount = 0;
    uilist bot_menu;
    bot_menu.text = string_format(
                        _( "Welcome to the %s Friendship Interface.  What would you like to do?\n"
                           "Your current friendship will last: %s" ), z.get_name(), to_string( friend_time ) );
    if( charge_count > 0 ) {
        bot_menu.addentry( 1, true, 'b', _( "Get more friendship.  10 cents/min" ) );
    } else {
        bot_menu.addentry( 2, true, 'q',
                           _( "Sadly you're not currently able to extend your friendship.  - Quit menu" ) );
    }
    bot_menu.query();
    switch( bot_menu.ret ) {
        case 1:
            amount = prompt_for_amount(
                         n_gettext( "How much friendship do you get?  Max: %d minute.  (0 to cancel)",
                                    "How much friendship do you get?  Max: %d minutes.", charge_count / 10 ), charge_count / 10 );
            if( amount > 0 ) {
                time_duration time_bought = time_duration::from_minutes( amount );
                player_character.use_charges( itype_cash_card, amount * 10 );
                z.add_effect( effect_pet, time_bought );
                z.add_effect( effect_paid, time_bought, true );
                z.friendly = -1;
                popup( _( "Your friendship grows stronger!\n This %s will follow you for %s." ), z.get_name(),
                       to_string( z.get_effect_dur( effect_pet ) ) );
                return true;
            }
            break;
        case 2:
            break;
    }

    return false;
}

bool monexamine::mfriend_menu( monster &z )
{
    enum choices {
        swap_pos = 0,
        push_monster,
        rename,
        attack,
        talk_to
    };

    uilist amenu;
    const std::string pet_name = z.get_name();

    amenu.text = string_format( _( "What to do with your %s?" ), pet_name );

    amenu.addentry( swap_pos, true, 's', _( "Swap positions" ) );
    amenu.addentry( push_monster, true, 'p', _( "Push %s" ), pet_name );
    amenu.addentry( rename, true, 'e', _( "Rename" ) );
    amenu.addentry( attack, true, 'a', _( "Attack" ) );
    if( !z.type->chat_topics.empty() ) {
        amenu.addentry( talk_to, true, 'c', _( "Talk to %s" ), pet_name );
    }
    amenu.query();
    const int choice = amenu.ret;

    switch( choice ) {
        case swap_pos:
            swap( z );
            break;
        case push_monster:
            push( z );
            break;
        case rename:
            rename_pet( z );
            break;
        case attack:
            if( query_yn( _( "You may be attacked!  Proceed?" ) ) ) {
                get_player_character().melee_attack( z, true );
            }
            break;
        case talk_to:
            get_avatar().talk_to( get_talker_for( z ) );
            break;
        default:
            break;
    }

    return true;
}
