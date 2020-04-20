#include "npctalk.h" // IWYU pragma: associated

#include <algorithm>
#include <cstddef>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "auto_pickup.h"
#include "avatar.h"
#include "basecamp.h"
#include "bionics.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character_id.h"
#include "character_martial_arts.h"
#include "debug.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "faction.h"
#include "game.h"
#include "game_constants.h"
#include "game_inventory.h"
#include "item.h"
#include "item_location.h"
#include "line.h"
#include "magic.h"
#include "map.h"
#include "memory_fast.h"
#include "messages.h"
#include "mission.h"
#include "monster.h"
#include "morale_types.h"
#include "mutation.h"
#include "npc.h"
#include "npctrade.h"
#include "optional.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "pimpl.h"
#include "player.h"
#include "player_activity.h"
#include "pldata.h"
#include "point.h"
#include "rng.h"
#include "string_id.h"
#include "translations.h"
#include "ui.h"

static const activity_id ACT_FIND_MOUNT( "ACT_FIND_MOUNT" );
static const activity_id ACT_MOVE_LOOT( "ACT_MOVE_LOOT" );
static const activity_id ACT_MULTIPLE_BUTCHER( "ACT_MULTIPLE_BUTCHER" );
static const activity_id ACT_MULTIPLE_CHOP_PLANKS( "ACT_MULTIPLE_CHOP_PLANKS" );
static const activity_id ACT_MULTIPLE_CHOP_TREES( "ACT_MULTIPLE_CHOP_TREES" );
static const activity_id ACT_MULTIPLE_CONSTRUCTION( "ACT_MULTIPLE_CONSTRUCTION" );
static const activity_id ACT_MULTIPLE_FARM( "ACT_MULTIPLE_FARM" );
static const activity_id ACT_MULTIPLE_FISH( "ACT_MULTIPLE_FISH" );
static const activity_id ACT_MULTIPLE_MINE( "ACT_MULTIPLE_MINE" );
static const activity_id ACT_VEHICLE_DECONSTRUCTION( "ACT_VEHICLE_DECONSTRUCTION" );
static const activity_id ACT_VEHICLE_REPAIR( "ACT_VEHICLE_REPAIR" );
static const activity_id ACT_WAIT_NPC( "ACT_WAIT_NPC" );
static const activity_id ACT_SOCIALIZE( "ACT_SOCIALIZE" );
static const activity_id ACT_TRAIN( "ACT_TRAIN" );

static const efftype_id effect_allow_sleep( "allow_sleep" );
static const efftype_id effect_asked_for_item( "asked_for_item" );
static const efftype_id effect_asked_personal_info( "asked_personal_info" );
static const efftype_id effect_asked_to_follow( "asked_to_follow" );
static const efftype_id effect_asked_to_lead( "asked_to_lead" );
static const efftype_id effect_asked_to_train( "asked_to_train" );
static const efftype_id effect_bite( "bite" );
static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_currently_busy( "currently_busy" );
static const efftype_id effect_infected( "infected" );
static const efftype_id effect_lying_down( "lying_down" );
static const efftype_id effect_npc_suspend( "npc_suspend" );
static const efftype_id effect_pet( "pet" );
static const efftype_id effect_sleep( "sleep" );

static const mtype_id mon_chicken( "mon_chicken" );
static const mtype_id mon_cow( "mon_cow" );
static const mtype_id mon_horse( "mon_horse" );

static const bionic_id bio_power_storage( "bio_power_storage" );
static const bionic_id bio_power_storage_mkII( "bio_power_storage_mkII" );

struct itype;

void spawn_animal( npc &p, const mtype_id &mon );

void talk_function::nothing( npc & )
{
}

void talk_function::assign_mission( npc &p )
{
    mission *miss = p.chatbin.mission_selected;
    if( miss == nullptr ) {
        debugmsg( "assign_mission: mission_selected == nullptr" );
        return;
    }
    miss->assign( g->u );
    p.chatbin.missions_assigned.push_back( miss );
    const auto it = std::find( p.chatbin.missions.begin(), p.chatbin.missions.end(), miss );
    p.chatbin.missions.erase( it );
}

void talk_function::mission_success( npc &p )
{
    mission *miss = p.chatbin.mission_selected;
    if( miss == nullptr ) {
        debugmsg( "mission_success: mission_selected == nullptr" );
        return;
    }

    int miss_val = npc_trading::cash_to_favor( p, miss->get_value() );
    npc_opinion tmp( 0, 0, 1 + miss_val / 5, -1, 0 );
    p.op_of_u += tmp;
    faction *p_fac = p.get_faction();
    if( p_fac != nullptr ) {
        int fac_val = std::min( 1 + miss_val / 10, 10 );
        p_fac->likes_u += fac_val;
        p_fac->respects_u += fac_val;
        p_fac->power += fac_val;
    }
    miss->wrap_up();
}

void talk_function::mission_failure( npc &p )
{
    mission *miss = p.chatbin.mission_selected;
    if( miss == nullptr ) {
        debugmsg( "mission_failure: mission_selected == nullptr" );
        return;
    }
    npc_opinion tmp( -1, 0, -1, 1, 0 );
    p.op_of_u += tmp;
    miss->fail();
}

void talk_function::clear_mission( npc &p )
{
    mission *miss = p.chatbin.mission_selected;
    if( miss == nullptr ) {
        debugmsg( "clear_mission: mission_selected == nullptr" );
        return;
    }
    const auto it = std::find( p.chatbin.missions_assigned.begin(), p.chatbin.missions_assigned.end(),
                               miss );
    if( it == p.chatbin.missions_assigned.end() ) {
        debugmsg( "clear_mission: mission_selected not in assigned" );
        return;
    }
    p.chatbin.missions_assigned.erase( it );
    if( p.chatbin.missions_assigned.empty() ) {
        p.chatbin.mission_selected = nullptr;
    } else {
        p.chatbin.mission_selected = p.chatbin.missions_assigned.front();
    }
    if( miss->has_follow_up() ) {
        p.add_new_mission( mission::reserve_new( miss->get_follow_up(), p.getID() ) );
        if( !p.chatbin.mission_selected ) {
            p.chatbin.mission_selected = p.chatbin.missions.front();
        }
    }
}

void talk_function::mission_reward( npc &p )
{
    const mission *miss = p.chatbin.mission_selected;
    if( miss == nullptr ) {
        debugmsg( "Called mission_reward with null mission" );
        return;
    }

    int mission_value = miss->get_value();
    p.op_of_u.owed += mission_value;
    npc_trading::trade( p, 0, _( "Reward" ) );
}

void talk_function::buy_chicken( npc &p )
{
    spawn_animal( p, mon_chicken );
}
void talk_function::buy_horse( npc &p )
{
    spawn_animal( p, mon_horse );
}

void talk_function::buy_cow( npc &p )
{
    spawn_animal( p, mon_cow );
}

void spawn_animal( npc &p, const mtype_id &mon )
{
    if( monster *const mon_ptr = g->place_critter_around( mon, p.pos(), 1 ) ) {
        mon_ptr->friendly = -1;
        mon_ptr->add_effect( effect_pet, 1_turns, num_bp, true );
    } else {
        // TODO: handle this gracefully (return the money, proper in-character message from npc)
        add_msg( m_debug, "No space to spawn purchased pet" );
    }
}

void talk_function::start_trade( npc &p )
{
    npc_trading::trade( p, 0, _( "Trade" ) );
}

void talk_function::sort_loot( npc &p )
{
    p.assign_activity( ACT_MOVE_LOOT );
}

void talk_function::do_construction( npc &p )
{
    p.assign_activity( ACT_MULTIPLE_CONSTRUCTION );
}

void talk_function::do_mining( npc &p )
{
    p.assign_activity( ACT_MULTIPLE_MINE );
}

void talk_function::do_read( npc &p )
{
    p.do_npc_read();
}

void talk_function::dismount( npc &p )
{
    p.npc_dismount();
}

void talk_function::find_mount( npc &p )
{
    // first find one nearby
    for( monster &critter : g->all_monsters() ) {
        if( p.can_mount( critter ) ) {
            // keep the horse still for some time, so that NPC can catch up to it and mount it.
            p.assign_activity( ACT_FIND_MOUNT );
            p.chosen_mount = g->shared_from( critter );
            // we found one, that's all we need.
            return;
        }
    }
    // if we got here and this was prompted by a renewal of the activity, and there are no valid monsters nearby, then cancel whole thing.
    if( p.has_player_activity() ) {
        p.revert_after_activity();
    }
}

void talk_function::do_butcher( npc &p )
{
    p.assign_activity( ACT_MULTIPLE_BUTCHER );
}

void talk_function::do_chop_plank( npc &p )
{
    p.assign_activity( ACT_MULTIPLE_CHOP_PLANKS );
}

void talk_function::do_vehicle_deconstruct( npc &p )
{
    p.assign_activity( ACT_VEHICLE_DECONSTRUCTION );
}

void talk_function::do_vehicle_repair( npc &p )
{
    p.assign_activity( ACT_VEHICLE_REPAIR );
}

void talk_function::do_chop_trees( npc &p )
{
    p.assign_activity( ACT_MULTIPLE_CHOP_TREES );
}

void talk_function::do_farming( npc &p )
{
    p.assign_activity( ACT_MULTIPLE_FARM );
}

void talk_function::do_fishing( npc &p )
{
    p.assign_activity( ACT_MULTIPLE_FISH );
}

void talk_function::revert_activity( npc &p )
{
    p.revert_after_activity();
}

void talk_function::goto_location( npc &p )
{
    int i = 0;
    uilist selection_menu;
    selection_menu.text = _( "Select a destination" );
    std::vector<basecamp *> camps;
    tripoint destination;
    for( auto elem : g->u.camps ) {
        if( elem == p.global_omt_location() ) {
            continue;
        }
        cata::optional<basecamp *> camp = overmap_buffer.find_camp( elem.xy() );
        if( !camp ) {
            continue;
        }
        basecamp *temp_camp = *camp;
        camps.push_back( temp_camp );
    }
    for( auto iter : camps ) {
        //~ %1$s: camp name, %2$d and %3$d: coordinates
        selection_menu.addentry( i++, true, MENU_AUTOASSIGN, pgettext( "camp", "%1$s at (%2$d, %3$d)" ),
                                 iter->camp_name(), iter->camp_omt_pos().x, iter->camp_omt_pos().y );
    }
    selection_menu.addentry( i++, true, MENU_AUTOASSIGN, _( "My current location" ) );
    selection_menu.addentry( i, true, MENU_AUTOASSIGN, _( "Cancel" ) );
    selection_menu.selected = 0;
    selection_menu.query();
    auto index = selection_menu.ret;
    if( index < 0 || index > static_cast<int>( camps.size() + 1 ) ||
        index == static_cast<int>( camps.size() + 1 ) ) {
        return;
    }
    if( index == static_cast<int>( camps.size() ) ) {
        destination = g->u.global_omt_location();
    } else {
        auto selected_camp = camps[index];
        destination = selected_camp->camp_omt_pos();
    }
    p.goal = destination;
    p.omt_path = overmap_buffer.get_npc_path( p.global_omt_location(), p.goal );
    if( destination == tripoint_zero || destination == overmap::invalid_tripoint ||
        p.omt_path.empty() ) {
        p.goal = npc::no_goal_point;
        p.omt_path.clear();
        add_msg( m_info, _( "That is not a valid destination for %s." ), p.disp_name() );
        return;
    }
    p.set_mission( NPC_MISSION_TRAVELLING );
    p.chatbin.first_topic = "TALK_FRIEND_GUARD";
    p.guard_pos = npc::no_goal_point;
    p.set_attitude( NPCATT_NULL );
}

void talk_function::assign_guard( npc &p )
{
    if( !p.is_player_ally() ) {
        p.set_mission( NPC_MISSION_GUARD );
        p.set_omt_destination();
        return;
    }

    if( p.has_player_activity() ) {
        p.revert_after_activity();
    }
    p.set_attitude( NPCATT_NULL );
    p.set_mission( NPC_MISSION_GUARD_ALLY );
    p.chatbin.first_topic = "TALK_FRIEND_GUARD";
    p.set_omt_destination();
}

void talk_function::abandon_camp( npc &p )
{
    cata::optional<basecamp *> bcp = overmap_buffer.find_camp( p.global_omt_location().xy() );
    if( bcp ) {
        basecamp *temp_camp = *bcp;
        temp_camp->abandon_camp();
    }
}

void talk_function::assign_camp( npc &p )
{
    cata::optional<basecamp *> bcp = overmap_buffer.find_camp( p.global_omt_location().xy() );
    if( bcp ) {
        basecamp *temp_camp = *bcp;
        p.set_attitude( NPCATT_NULL );
        p.set_mission( NPC_MISSION_GUARD_ALLY );
        temp_camp->add_assignee( p.getID() );
        temp_camp->job_assignment_ui();
        temp_camp->validate_assignees();
        add_msg( _( "%1$s is assigned to %2$s" ), p.disp_name(), temp_camp->camp_name() );
        if( p.has_player_activity() ) {
            p.revert_after_activity();
        }
        p.chatbin.first_topic = "TALK_FRIEND_GUARD";
        p.set_omt_destination();
    }
}

void talk_function::stop_guard( npc &p )
{
    if( !p.is_player_ally() ) {
        p.set_attitude( NPCATT_NULL );
        p.set_mission( NPC_MISSION_NULL );
        return;
    }
    p.set_attitude( NPCATT_FOLLOW );
    add_msg( _( "%s begins to follow you." ), p.name );
    p.set_mission( NPC_MISSION_NULL );
    if( p.has_companion_mission() ) {
        p.reset_companion_mission();
    }
    p.chatbin.first_topic = "TALK_FRIEND";
    p.goal = npc::no_goal_point;
    p.guard_pos = npc::no_goal_point;
    if( p.assigned_camp ) {
        if( cata::optional<basecamp *> bcp = overmap_buffer.find_camp( ( *p.assigned_camp ).xy() ) ) {
            ( *bcp )->remove_assignee( p.getID() );
            ( *bcp )->validate_assignees();
        }
        p.assigned_camp = cata::nullopt;
    }
}

void talk_function::wake_up( npc &p )
{
    p.rules.clear_override( ally_rule::allow_sleep );
    p.rules.enable_override( ally_rule::allow_sleep );
    p.remove_effect( effect_allow_sleep );
    p.remove_effect( effect_lying_down );
    p.remove_effect( effect_npc_suspend );
    p.remove_effect( effect_sleep );
    // TODO: Get mad at player for waking us up unless we're in danger
}

void talk_function::reveal_stats( npc &p )
{
    p.disp_info();
}

void talk_function::end_conversation( npc &p )
{
    add_msg( _( "%s starts ignoring you." ), p.name );
    p.chatbin.first_topic = "TALK_DONE";
}

void talk_function::insult_combat( npc &p )
{
    add_msg( _( "You start a fight with %s!" ), p.name );
    p.chatbin.first_topic = "TALK_DONE";
    p.set_attitude( NPCATT_KILL );
}

void talk_function::bionic_install( npc &p )
{
    item_location bionic = game_menus::inv::install_bionic( g->u, g->u, true );

    if( !bionic ) {
        return;
    }

    const item *tmp = bionic.get_item();
    const itype &it = *tmp->type;

    signed int price = tmp->price( true ) * 2;
    if( !npc_trading::pay_npc( p, price ) ) {
        return;
    }

    //Makes the doctor awesome at installing but not perfect
    if( g->u.can_install_bionics( it, p, false, 20 ) ) {
        bionic.remove_item();
        g->u.install_bionics( it, p, false, 20 );
    }
}

void talk_function::bionic_remove( npc &p )
{
    const bionic_collection all_bio = *g->u.my_bionics;
    if( all_bio.empty() ) {
        popup( _( "You don't have any bionics installed…" ) );
        return;
    }

    std::vector<itype_id> bionic_types;
    std::vector<std::string> bionic_names;
    for( const bionic &bio : all_bio ) {
        if( std::find( bionic_types.begin(), bionic_types.end(), bio.id.str() ) == bionic_types.end() ) {
            if( bio.id != bio_power_storage ||
                bio.id != bio_power_storage_mkII ) {
                bionic_types.push_back( bio.id.str() );
                if( item::type_is_defined( bio.id.str() ) ) {
                    item tmp = item( bio.id.str(), 0 );
                    bionic_names.push_back( tmp.tname() + " - " + format_money( 50000 + ( tmp.price( true ) / 4 ) ) );
                } else {
                    bionic_names.push_back( bio.id.str() + " - " + format_money( 50000 ) );
                }
            }
        }
    }
    // Choose bionic if applicable
    int bionic_index = uilist( _( "Which bionic do you wish to uninstall?" ),
                               bionic_names );
    // Did we cancel?
    if( bionic_index < 0 ) {
        popup( _( "You decide to hold off…" ) );
        return;
    }

    signed int price;
    if( item::type_is_defined( bionic_types[bionic_index] ) ) {
        price = 50000 + ( item( bionic_types[bionic_index], 0 ).price( true ) / 4 );
    } else {
        price = 50000;
    }
    if( !npc_trading::pay_npc( p, price ) ) {
        return;
    }

    //Makes the doctor awesome at installing but not perfect
    if( g->u.can_uninstall_bionic( bionic_id( bionic_types[bionic_index] ), p, false ) ) {
        g->u.amount_of( bionic_types[bionic_index] ); // ??? this does nothing, it just queries the count
        g->u.uninstall_bionic( bionic_id( bionic_types[bionic_index] ), p, false );
    }

}

void talk_function::give_equipment( npc &p )
{
    std::vector<item_pricing> giving = npc_trading::init_selling( p );
    int chosen = -1;
    while( chosen == -1 && !giving.empty() ) {
        int index = rng( 0, giving.size() - 1 );
        if( giving[index].price < p.op_of_u.owed ) {
            chosen = index;
        } else {
            giving.erase( giving.begin() + index );
        }
    }
    if( giving.empty() ) {
        popup( _( "%s has nothing to give!" ), p.name );
        return;
    }
    if( chosen < 0 || static_cast<size_t>( chosen ) >= giving.size() ) {
        debugmsg( "Chosen index is outside of available item range!" );
        chosen = 0;
    }
    item it = *giving[chosen].loc.get_item();
    giving[chosen].loc.remove_item();
    popup( _( "%1$s gives you a %2$s" ), p.name, it.tname() );
    it.set_owner( g->u );
    g->u.i_add( it );
    p.op_of_u.owed -= giving[chosen].price;
    p.add_effect( effect_asked_for_item, 3_hours );
}

void talk_function::give_aid( npc &p )
{
    p.add_effect( effect_currently_busy, 30_minutes );
    for( int i = 0; i < num_hp_parts; i++ ) {
        const body_part bp_healed = player::hp_to_bp( static_cast<hp_part>( i ) );
        g->u.heal( static_cast<hp_part>( i ), 5 * rng( 2, 5 ) );
        if( g->u.has_effect( effect_bite, bp_healed ) ) {
            g->u.remove_effect( effect_bite, bp_healed );
        }
        if( g->u.has_effect( effect_bleed, bp_healed ) ) {
            g->u.remove_effect( effect_bleed, bp_healed );
        }
        if( g->u.has_effect( effect_infected, bp_healed ) ) {
            g->u.remove_effect( effect_infected, bp_healed );
        }
    }
    const int moves = to_moves<int>( 100_minutes );
    g->u.assign_activity( ACT_WAIT_NPC, moves );
    g->u.activity.str_values.push_back( p.name );
}

void talk_function::give_all_aid( npc &p )
{
    p.add_effect( effect_currently_busy, 30_minutes );
    give_aid( p );
    for( npc &guy : g->all_npcs() ) {
        if( guy.is_walking_with() && rl_dist( guy.pos(), g->u.pos() ) < PICKUP_RANGE ) {
            for( int i = 0; i < num_hp_parts; i++ ) {
                const body_part bp_healed = player::hp_to_bp( static_cast<hp_part>( i ) );
                guy.heal( static_cast<hp_part>( i ), 5 * rng( 2, 5 ) );
                if( guy.has_effect( effect_bite, bp_healed ) ) {
                    guy.remove_effect( effect_bite, bp_healed );
                }
                if( guy.has_effect( effect_bleed, bp_healed ) ) {
                    guy.remove_effect( effect_bleed, bp_healed );
                }
                if( guy.has_effect( effect_infected, bp_healed ) ) {
                    guy.remove_effect( effect_infected, bp_healed );
                }
            }
        }
    }
}

static void generic_barber( const std::string &mut_type )
{
    uilist hair_menu;
    std::string menu_text;
    if( mut_type == "hair_style" ) {
        menu_text = _( "Choose a new hairstyle" );
    } else if( mut_type == "facial_hair" ) {
        menu_text = _( "Choose a new facial hair style" );
    }
    hair_menu.text = menu_text;
    int index = 0;
    hair_menu.addentry( index, true, 'q', _( "Actually…  I've changed my mind." ) );
    std::vector<trait_id> hair_muts = get_mutations_in_type( mut_type );
    trait_id cur_hair;
    for( auto elem : hair_muts ) {
        if( g->u.has_trait( elem ) ) {
            cur_hair = elem;
        }
        index += 1;
        hair_menu.addentry( index, true, MENU_AUTOASSIGN, elem.obj().name() );
    }
    hair_menu.query();
    int choice = hair_menu.ret;
    if( choice != 0 ) {
        if( g->u.has_trait( cur_hair ) ) {
            g->u.remove_mutation( cur_hair, true );
        }
        g->u.set_mutation( hair_muts[ choice - 1 ] );
        add_msg( m_info, _( "You get a trendy new cut!" ) );
    }
}

void talk_function::barber_beard( npc &/*p*/ )
{
    generic_barber( "facial_hair" );
}

void talk_function::barber_hair( npc &/*p*/ )
{
    generic_barber( "hair_style" );
}

void talk_function::buy_haircut( npc &p )
{
    g->u.add_morale( MORALE_HAIRCUT, 5, 5, 720_minutes, 3_minutes );
    const int moves = to_moves<int>( 20_minutes );
    g->u.assign_activity( ACT_WAIT_NPC, moves );
    g->u.activity.str_values.push_back( p.name );
    add_msg( m_good, _( "%s gives you a decent haircut…" ), p.name );
}

void talk_function::buy_shave( npc &p )
{
    g->u.add_morale( MORALE_SHAVE, 10, 10, 360_minutes, 3_minutes );
    const int moves = to_moves<int>( 5_minutes );
    g->u.assign_activity( ACT_WAIT_NPC, moves );
    g->u.activity.str_values.push_back( p.name );
    add_msg( m_good, _( "%s gives you a decent shave…" ), p.name );
}

void talk_function::morale_chat( npc &p )
{
    g->u.add_morale( MORALE_CHAT, rng( 3, 10 ), 10, 200_minutes, 5_minutes / 2 );
    add_msg( m_good, _( "That was a pleasant conversation with %s…" ), p.disp_name() );
}

void talk_function::morale_chat_activity( npc &p )
{
    const int moves = to_moves<int>( 10_minutes );
    g->u.assign_activity( ACT_SOCIALIZE, moves );
    g->u.activity.str_values.push_back( p.name );
    add_msg( m_good, _( "That was a pleasant conversation with %s." ), p.disp_name() );
    g->u.add_morale( MORALE_CHAT, rng( 3, 10 ), 10, 200_minutes, 5_minutes / 2 );
}

void talk_function::buy_10_logs( npc &p )
{
    std::vector<tripoint> places = overmap_buffer.find_all(
                                       g->u.global_omt_location(), "ranch_camp_67", 1, false );
    if( places.empty() ) {
        debugmsg( "Couldn't find %s", "ranch_camp_67" );
        return;
    }
    const auto &cur_om = g->get_cur_om();
    std::vector<tripoint> places_om;
    for( auto &i : places ) {
        if( &cur_om == overmap_buffer.get_existing_om_global( i ).om ) {
            places_om.push_back( i );
        }
    }

    const tripoint site = random_entry( places_om );
    tinymap bay;
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    bay.spawn_item( point( 7, 15 ), "log", 10 );
    bay.save();

    p.add_effect( effect_currently_busy, 1_days );
    add_msg( m_good, _( "%s drops the logs off in the garage…" ), p.name );
}

void talk_function::buy_100_logs( npc &p )
{
    std::vector<tripoint> places = overmap_buffer.find_all(
                                       g->u.global_omt_location(), "ranch_camp_67", 1, false );
    if( places.empty() ) {
        debugmsg( "Couldn't find %s", "ranch_camp_67" );
        return;
    }
    const auto &cur_om = g->get_cur_om();
    std::vector<tripoint> places_om;
    for( auto &i : places ) {
        if( &cur_om == overmap_buffer.get_existing_om_global( i ).om ) {
            places_om.push_back( i );
        }
    }

    const tripoint site = random_entry( places_om );
    tinymap bay;
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    bay.spawn_item( point( 7, 15 ), "log", 100 );
    bay.save();

    p.add_effect( effect_currently_busy, 7_days );
    add_msg( m_good, _( "%s drops the logs off in the garage…" ), p.name );
}

void talk_function::follow( npc &p )
{
    g->add_npc_follower( p.getID() );
    p.set_attitude( NPCATT_FOLLOW );
    p.set_fac( faction_id( "your_followers" ) );
    g->u.cash += p.cash;
    p.cash = 0;
}

void talk_function::follow_only( npc &p )
{
    p.set_attitude( NPCATT_FOLLOW );
}

void talk_function::deny_follow( npc &p )
{
    p.add_effect( effect_asked_to_follow, 6_hours );
}

void talk_function::deny_lead( npc &p )
{
    p.add_effect( effect_asked_to_lead, 6_hours );
}

void talk_function::deny_equipment( npc &p )
{
    p.add_effect( effect_asked_for_item, 1_hours );
}

void talk_function::deny_train( npc &p )
{
    p.add_effect( effect_asked_to_train, 6_hours );
}

void talk_function::deny_personal_info( npc &p )
{
    p.add_effect( effect_asked_personal_info, 3_hours );
}

void talk_function::hostile( npc &p )
{
    if( p.get_attitude() == NPCATT_KILL ) {
        return;
    }

    if( p.sees( g->u ) ) {
        add_msg( _( "%s turns hostile!" ), p.name );
    }

    g->events().send<event_type::npc_becomes_hostile>( p.getID(), p.name );
    p.set_attitude( NPCATT_KILL );
}

void talk_function::flee( npc &p )
{
    add_msg( _( "%s turns to flee!" ), p.name );
    p.set_attitude( NPCATT_FLEE );
}

void talk_function::leave( npc &p )
{
    add_msg( _( "%s leaves." ), p.name );
    g->remove_npc_follower( p.getID() );
    std::string new_fac_id = "solo_";
    new_fac_id += p.name;
    p.job.clear_all_priorities();
    // create a new "lone wolf" faction for this one NPC
    faction *new_solo_fac = g->faction_manager_ptr->add_new_faction( p.name,
                            faction_id( new_fac_id ), faction_id( "no_faction" ) );
    p.set_fac( new_solo_fac ? new_solo_fac->id : faction_id( "no_faction" ) );
    if( new_solo_fac ) {
        new_solo_fac->known_by_u = true;
    }
    p.chatbin.first_topic = "TALK_STRANGER_NEUTRAL";
    p.set_attitude( NPCATT_NULL );
    p.mission = NPC_MISSION_NULL;
    p.long_term_goal_action();
}

void talk_function::stop_following( npc &p )
{
    // this is to tell non-allied NPCs to stop following.
    // ( usually after a mission where they were temporarily tagging along )
    // so don't tell already allied NPCs to stop following.
    // they use the guard command for that.
    if( p.is_player_ally() ) {
        return;
    }
    add_msg( _( "%s stops following." ), p.name );
    p.set_attitude( NPCATT_NULL );
}

void talk_function::stranger_neutral( npc &p )
{
    add_msg( _( "%s feels less threatened by you." ), p.name );
    p.set_attitude( NPCATT_NULL );
    p.chatbin.first_topic = "TALK_STRANGER_NEUTRAL";
}

void talk_function::drop_stolen_item( npc &p )
{
    for( auto &elem : g->u.inv_dump() ) {
        if( elem->is_old_owner( p ) ) {
            item to_drop = g->u.i_rem( elem );
            to_drop.remove_old_owner();
            to_drop.set_owner( p );
            g->m.add_item_or_charges( g->u.pos(), to_drop );
        }
    }
    if( p.known_stolen_item ) {
        p.known_stolen_item = nullptr;
    }
    if( g->u.is_hauling() ) {
        g->u.stop_hauling();
    }
    p.set_attitude( NPCATT_NULL );
}

void talk_function::remove_stolen_status( npc &p )
{
    if( p.known_stolen_item ) {
        p.known_stolen_item = nullptr;
    }
    p.set_attitude( NPCATT_NULL );
}

void talk_function::start_mugging( npc &p )
{
    p.set_attitude( NPCATT_MUG );
    add_msg( _( "Pause to stay still.  Any movement may cause %s to attack." ), p.name );
}

void talk_function::player_leaving( npc &p )
{
    p.set_attitude( NPCATT_WAIT_FOR_LEAVE );
    p.patience = 15 - p.personality.aggression;
}

void talk_function::drop_weapon( npc &p )
{
    if( p.is_hallucination() ) {
        return;
    }
    g->m.add_item_or_charges( p.pos(), p.remove_weapon() );
}

void talk_function::player_weapon_away( npc &/*p*/ )
{
    g->u.i_add( g->u.remove_weapon() );
}

void talk_function::player_weapon_drop( npc &/*p*/ )
{
    g->m.add_item_or_charges( g->u.pos(), g->u.remove_weapon() );
}

void talk_function::lead_to_safety( npc &p )
{
    const auto mission = mission::reserve_new( mission_type_id( "MISSION_REACH_SAFETY" ),
                         character_id() );
    mission->assign( g->u );
    p.goal = mission->get_target();
    p.set_attitude( NPCATT_LEAD );
}

bool npc_trading::pay_npc( npc &np, int cost )
{
    if( np.op_of_u.owed >= cost ) {
        np.op_of_u.owed -= cost;
        return true;
    }

    return npc_trading::trade( np, cost, _( "Pay:" ) );
}

void talk_function::start_training( npc &p )
{
    int cost;
    time_duration time = 0_turns;
    std::string name;
    const skill_id &skill = p.chatbin.skill;
    const matype_id &style = p.chatbin.style;
    const spell_id &sp_id = p.chatbin.dialogue_spell;
    int expert_multiplier = 1;
    if( skill.is_valid() && g->u.get_skill_level( skill ) < p.get_skill_level( skill ) ) {
        cost = calc_skill_training_cost( p, skill );
        time = calc_skill_training_time( p, skill );
        name = skill.str();
    } else if( p.chatbin.style.is_valid() && !g->u.martial_arts_data.has_martialart( style ) ) {
        cost = calc_ma_style_training_cost( p, style );
        time = calc_ma_style_training_time( p, style );
        name = p.chatbin.style.str();
        // already checked if can learn this spell in npctalk.cpp
    } else if( p.chatbin.dialogue_spell.is_valid() ) {
        const spell &temp_spell = p.magic.get_spell( sp_id );
        const bool knows = g->u.magic.knows_spell( sp_id );
        cost = p.calc_spell_training_cost( knows, temp_spell.get_difficulty(), temp_spell.get_level() );
        name = temp_spell.id().str();
        expert_multiplier = knows ? temp_spell.get_level() - g->u.magic.get_spell( sp_id ).get_level() : 1;
        // quicker to learn with instruction as opposed to books.
        // if this is a known spell, then there is a set time to gain some exp.
        // if player doesn't know this spell, then the NPC will teach all of it
        // which takes max 6 hours, min 3 hours.
        // TODO: a system for NPCs to train new stuff in bits and pieces
        // and remember the progress.
        if( knows ) {
            time = 1_hours;
        } else {
            time = time_duration::from_seconds( clamp( g->u.magic.time_to_learn_spell( g->u, sp_id ) / 50, 7200,
                                                21600 ) );
        }
    } else {
        debugmsg( "start_training with no valid skill or style set" );
        return;
    }

    mission *miss = p.chatbin.mission_selected;
    if( miss != nullptr && miss->get_assigned_player_id() == g->u.getID() &&
        miss->is_complete( g->u.getID() ) ) {
        clear_mission( p );
    } else if( !npc_trading::pay_npc( p, cost ) ) {
        return;
    }
    player_activity act = player_activity( ACT_TRAIN, to_moves<int>( time ),
                                           p.getID().get_value(), 0, name );
    act.values.push_back( expert_multiplier );
    g->u.assign_activity( act );

    p.add_effect( effect_asked_to_train, 6_hours );
}

npc *pick_follower()
{
    std::vector<npc *> followers;
    std::vector<tripoint> locations;

    for( npc &guy : g->all_npcs() ) {
        if( guy.is_player_ally() && g->u.sees( guy ) ) {
            followers.push_back( &guy );
            locations.push_back( guy.pos() );
        }
    }

    pointmenu_cb callback( locations );

    uilist menu;
    menu.text = _( "Select a follower" );
    menu.callback = &callback;
    menu.w_y = 2;

    for( const npc *p : followers ) {
        menu.addentry( -1, true, MENU_AUTOASSIGN, p->name );
    }

    menu.query();
    if( menu.ret < 0 || static_cast<size_t>( menu.ret ) >= followers.size() ) {
        return nullptr;
    }

    return followers[ menu.ret ];
}

void talk_function::copy_npc_rules( npc &p )
{
    const npc *other = pick_follower();
    if( other != nullptr && other != &p ) {
        p.rules = other->rules;
    }
}

void talk_function::set_npc_pickup( npc &p )
{
    p.rules.pickup_whitelist->show( p.name );
}

void talk_function::npc_die( npc &p )
{
    p.die( nullptr );
    const shared_ptr_fast<npc> guy = overmap_buffer.find_npc( p.getID() );
    if( guy && !guy->is_dead() ) {
        guy->marked_for_death = true;
    }
}

void talk_function::npc_thankful( npc &p )
{
    if( p.get_attitude() == NPCATT_MUG || p.get_attitude() == NPCATT_WAIT_FOR_LEAVE ||
        p.get_attitude() == NPCATT_FLEE || p.get_attitude() == NPCATT_KILL ||
        p.get_attitude() == NPCATT_FLEE_TEMP ) {
        p.set_attitude( NPCATT_NULL );
    }
    if( p.chatbin.first_topic != "TALK_FRIEND" ) {
        p.chatbin.first_topic = "TALK_STRANGER_FRIENDLY";
    }
    p.personality.aggression -= 1;

}

void talk_function::clear_overrides( npc &p )
{
    p.rules.clear_overrides();
}
