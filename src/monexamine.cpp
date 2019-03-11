#include "monexamine.h"

#include <string>
#include <utility>

#include "calendar.h"
#include "game.h"
#include "game_inventory.h"
#include "item.h"
#include "iuse.h"
#include "map.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "player.h"
#include "output.h"
#include "string_input_popup.h"
#include "translations.h"
#include "ui.h"

const species_id ZOMBIE( "ZOMBIE" );

const efftype_id effect_controlled( "controlled" );
const efftype_id effect_has_bag( "has_bag" );
const efftype_id effect_milked( "milked" );
const efftype_id effect_tied( "tied" );

bool monexamine::pet_menu( monster &z )
{
    enum choices {
        swap_pos = 0,
        push_zlave,
        rename,
        attach_bag,
        drop_all,
        give_items,
        play_with_pet,
        pheromone,
        milk,
        rope
    };

    uilist amenu;
    std::string pet_name = z.get_name();
    bool is_zombie = z.type->in_species( ZOMBIE );
    if( is_zombie ) {
        pet_name = _( "zombie slave" );
    }

    amenu.text = string_format( _( "What to do with your %s?" ), pet_name );

    amenu.addentry( swap_pos, true, 's', _( "Swap positions" ) );
    amenu.addentry( push_zlave, true, 'p', _( "Push %s" ), pet_name );
    amenu.addentry( rename, true, 'e', _( "Rename" ) );
    if( z.has_effect( effect_has_bag ) ) {
        amenu.addentry( give_items, true, 'g', _( "Place items into bag" ) );
        amenu.addentry( drop_all, true, 'd', _( "Drop all items" ) );
    } else {
        amenu.addentry( attach_bag, true, 'b', _( "Attach bag" ) );
    }
    if( z.has_flag( MF_BIRDFOOD ) || z.has_flag( MF_CATFOOD ) || z.has_flag( MF_DOGFOOD ) ) {
        amenu.addentry( play_with_pet, true, 'y', _( "Play with %s" ), pet_name );
    }
    if( z.has_effect( effect_tied ) ) {
        amenu.addentry( rope, true, 'r', _( "Untie" ) );
    } else {
        if( g->u.has_amount( "rope_6", 1 ) ) {
            amenu.addentry( rope, true, 'r', _( "Tie" ) );
        } else {
            amenu.addentry( rope, false, 'r', _( "You need a short rope to tie %s in place" ),
                            pet_name );
        }
    }
    if( is_zombie ) {
        amenu.addentry( pheromone, true, 't', _( "Tear out pheromone ball" ) );
    }

    if( z.has_flag( MF_MILKABLE ) ) {
        amenu.addentry( milk, true, 'm', _( "Milk %s" ), pet_name );
    }

    amenu.query();
    int choice = amenu.ret;

    switch( choice ) {
        case swap_pos:
            swap( z );
            break;
        case push_zlave:
            push( z );
            break;
        case rename:
            rename_pet( z );
            break;
        case attach_bag:
            attach_bag_to( z );
            break;
        case drop_all:
            dump_items( z );
            break;
        case give_items:
            return give_items_to( z );
            break;
        case play_with_pet:
            if( query_yn( _( "Spend a few minutes to play with your %s?" ), pet_name ) ) {
                play_with( z );
            }
            break;
        case pheromone:
            if( query_yn( _( "Really kill the zombie slave?" ) ) ) {
                kill_zslave( z );
            }
            break;
        case rope:
            tie_or_untie( z );
            break;
        case milk:
            milk_source( z );
            break;
        default:
            break;
    }
    return true;
}

void monexamine::swap( monster &z )
{
    std::string pet_name = z.get_name();
    g->u.moves -= 150;

    ///\EFFECT_STR increases chance to successfully swap positions with your pet
    ///\EFFECT_DEX increases chance to successfully swap positions with your pet
    if( !one_in( ( g->u.str_cur + g->u.dex_cur ) / 6 ) ) {
        bool t = z.has_effect( effect_tied );
        if( t ) {
            z.remove_effect( effect_tied );
        }

        tripoint zp = z.pos();
        z.move_to( g->u.pos(), true );
        g->u.setpos( zp );

        if( t ) {
            z.add_effect( effect_tied, 1_turns, num_bp, true );
        }
        add_msg( _( "You swap positions with your %s." ), pet_name );
    } else {
        add_msg( _( "You fail to budge your %s!" ), pet_name );
    }
}

void monexamine::push( monster &z )
{
    std::string pet_name = z.get_name();
    g->u.moves -= 30;

    ///\EFFECT_STR increases chance to successfully push your pet
    if( !one_in( g->u.str_cur ) ) {
        add_msg( _( "You pushed the %s." ), pet_name );
    } else {
        add_msg( _( "You pushed the %s, but it resisted." ), pet_name );
        return;
    }

    int deltax = z.posx() - g->u.posx(), deltay = z.posy() - g->u.posy();
    z.move_to( tripoint( z.posx() + deltax, z.posy() + deltay, z.posz() ) );
}

void monexamine::rename_pet( monster &z )
{
    std::string unique_name = string_input_popup()
                              .title( _( "Enter new pet name:" ) )
                              .width( 20 )
                              .query_string();
    if( unique_name.length() > 0 ) {
        z.unique_name = unique_name;
    }
}

void monexamine::attach_bag_to( monster &z )
{
    std::string pet_name = z.get_name();
    int pos = g->inv_for_filter( _( "Bag item" ), []( const item & it ) {
        return it.is_armor() && it.get_storage() > 0_ml;
    } );

    if( pos == INT_MIN ) {
        add_msg( _( "Never mind." ) );
        return;
    }

    item &it = g->u.i_at( pos );
    z.add_item( it );
    add_msg( _( "You mount the %1$s on your %2$s, ready to store gear." ),
             it.display_name(), pet_name );
    g->u.i_rem( pos );
    z.add_effect( effect_has_bag, 1_turns, num_bp, true );
    g->u.moves -= 200;
}


void monexamine::dump_items( monster &z )
{
    std::string pet_name = z.get_name();
    for( auto &it : z.inv ) {
        g->m.add_item_or_charges( z.pos(), it );
    }

    z.inv.clear();
    z.remove_effect( effect_has_bag );
    add_msg( _( "You dump the contents of the %s's bag on the ground." ), pet_name );
    g->u.moves -= 200;
}

bool monexamine::give_items_to( monster &z )
{
    std::string pet_name = z.get_name();
    if( z.inv.empty() ) {
        add_msg( _( "There is no container on your %s to put things in!" ), pet_name );
        return true;
    }

    item &it = z.inv[0];

    if( !it.is_armor() ) {
        add_msg( _( "There is no container on your %s to put things in!" ), pet_name );
        return true;
    }

    units::volume max_cap = it.get_storage();
    units::mass max_weight = z.weight_capacity() - it.weight();

    if( z.inv.size() > 1 ) {
        for( auto &i : z.inv ) {
            max_cap -= i.volume();
            max_weight -= i.weight();
        }
    }

    if( max_weight <= 0_gram ) {
        add_msg( _( "%1$s is overburdened. You can't transfer your %2$s." ),
                 pet_name, it.tname( 1 ) );
        return true;
    }
    if( max_cap <= 0_ml ) {
        add_msg( _( "There's no room in your %1$s's %2$s for that, it's too bulky!" ),
                 pet_name, it.tname( 1 ) );
        return true;
    }

    const auto items_to_stash = game_menus::inv::multidrop( g->u );
    if( !items_to_stash.empty() ) {
        g->u.drop( items_to_stash, z.pos(), true );
        z.add_effect( effect_controlled, 5_turns );
        return true;
    }
    return false;
}

void monexamine::play_with( monster &z )
{
    std::string pet_name = z.get_name();
    g->u.assign_activity( activity_id( "ACT_PLAY_WITH_PET" ), rng( 50, 125 ) * 100 );
    g->u.activity.str_values.push_back( pet_name );
}

void monexamine::kill_zslave( monster &z )
{
    z.apply_damage( &g->u, bp_torso, 100 ); // damage the monster (and its corpse)
    z.die( &g->u ); // and make sure it's really dead

    g->u.moves -= 150;

    if( !one_in( 3 ) ) {
        g->u.add_msg_if_player( _( "You tear out the pheromone ball from the zombie slave." ) );
        item ball( "pheromone", 0 );
        iuse pheromone;
        pheromone.pheromone( &( g->u ), &ball, true, g->u.pos() );
    }
}

void monexamine::tie_or_untie( monster &z )
{
    if( z.has_effect( effect_tied ) ) {
        z.remove_effect( effect_tied );
        item rope_6( "rope_6", 0 );
        g->u.i_add( rope_6 );
    } else {
        z.add_effect( effect_tied, 1_turns, num_bp, true );
        g->u.use_amount( "rope_6", 1 );
    }
}

void monexamine::milk_source( monster &source_mon )
{
    const auto milked_item = source_mon.type->starting_ammo.find( "milk" );
    if( milked_item == source_mon.type->starting_ammo.end() ) {
        debugmsg( "%s is milkable but has no milk in its starting ammo!",
                  source_mon.get_name().c_str() );
        return;
    }
    const long milk_per_day = milked_item->second;
    const time_duration milking_freq = 1_days / milk_per_day;

    long remaining_milk = milk_per_day;
    if( source_mon.has_effect( effect_milked ) ) {
        remaining_milk -= source_mon.get_effect_dur( effect_milked ) / milking_freq;
    }


    if( remaining_milk > 0 ) {
        // pin the cow in place if it isn't already
        bool temp_tie = !source_mon.has_effect( effect_tied );
        if( temp_tie ) {
            source_mon.add_effect( effect_tied, 1_turns, num_bp, true );
        }

        item milk( milked_item->first, calendar::turn, remaining_milk );
        if( g->handle_liquid( milk, nullptr, 1, nullptr, nullptr, -1, &source_mon ) ) {
            add_msg( _( "You milk the %s." ), source_mon.get_name() );
            long transferred_milk = remaining_milk - milk.charges;
            source_mon.add_effect( effect_milked, milking_freq * transferred_milk );
            g->u.mod_moves( -to_moves<int>( transferred_milk * 1_minutes / 5 ) );
        }
        if( temp_tie ) {
            source_mon.remove_effect( effect_tied );
        }
    } else {
        add_msg( _( "The %s's udders run dry." ), source_mon.get_name() );
    }
}
