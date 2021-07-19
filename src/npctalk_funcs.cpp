#include "npctalk.h" // IWYU pragma: associated

#include <algorithm>
#include <cstddef>
#include <iosfwd>
#include <list>
#include <memory>
#include <new>
#include <set>
#include <string>
#include <vector>

#include "activity_type.h"
#include "auto_pickup.h"
#include "avatar.h"
#include "basecamp.h"
#include "bionics.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "character_id.h"
#include "character_martial_arts.h"
#include "coordinates.h"
#include "creature.h"
#include "debug.h"
#include "dialogue_chatbin.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "faction.h"
#include "game.h"
#include "game_constants.h"
#include "game_inventory.h"
#include "item.h"
#include "item_contents.h"
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
#include "player_activity.h"
#include "point.h"
#include "rng.h"
#include "translations.h"
#include "ui.h"
#include "viewer.h"

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

struct itype;

static void spawn_animal( npc &p, const mtype_id &mon );

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
    miss->assign( get_avatar() );
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
        mon_ptr->add_effect( effect_pet, 1_turns, true );
    } else {
        // TODO: handle this gracefully (return the money, proper in-character message from npc)
        add_msg_debug( debugmode::DF_NPC, "No space to spawn purchased pet" );
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
    tripoint_abs_omt destination;
    Character &player_character = get_player_character();
    for( auto elem : player_character.camps ) {
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
    for( const basecamp *iter : camps ) {
        //~ %1$s: camp name, %2$d and %3$d: coordinates
        selection_menu.addentry( i++, true, MENU_AUTOASSIGN, pgettext( "camp", "%1$s at %2$s" ),
                                 iter->camp_name(), iter->camp_omt_pos().to_string() );
    }
    selection_menu.addentry( i++, true, MENU_AUTOASSIGN, _( "My current location" ) );
    selection_menu.addentry( i, true, MENU_AUTOASSIGN, _( "Cancel" ) );
    selection_menu.selected = 0;
    selection_menu.query();
    int index = selection_menu.ret;
    if( index < 0 || index > static_cast<int>( camps.size() + 1 ) ||
        index == static_cast<int>( camps.size() + 1 ) ) {
        return;
    }
    if( index == static_cast<int>( camps.size() ) ) {
        destination = player_character.global_omt_location();
    } else {
        const basecamp *selected_camp = camps[index];
        destination = selected_camp->camp_omt_pos();
    }
    p.goal = destination;
    p.omt_path = overmap_buffer.get_npc_path( p.global_omt_location(), p.goal );
    if( destination == tripoint_abs_omt() || destination == overmap::invalid_tripoint ||
        p.omt_path.empty() ) {
        p.goal = npc::no_goal_point;
        p.omt_path.clear();
        add_msg( m_info, _( "That is not a valid destination for %s." ), p.disp_name() );
        return;
    }
    p.set_mission( NPC_MISSION_TRAVELLING );
    p.chatbin.first_topic = "TALK_FRIEND_GUARD";
    p.guard_pos = tripoint_min;
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
    p.guard_pos = tripoint_min;
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
    avatar &player_character = get_avatar();
    item_location bionic = game_menus::inv::install_bionic( player_character, player_character, true );

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
    if( player_character.can_install_bionics( it, p, false, 20 ) ) {
        bionic.remove_item();
        player_character.install_bionics( it, p, false, 20 );
    }
}

void talk_function::bionic_remove( npc &p )
{
    Character &player_character = get_player_character();
    const bionic_collection all_bio = *player_character.my_bionics;
    if( all_bio.empty() ) {
        popup( _( "You don't have any bionics installed…" ) );
        return;
    }

    std::vector<itype_id> bionic_types;
    std::vector<std::string> bionic_names;
    for( const bionic &bio : all_bio ) {
        if( std::find( bionic_types.begin(), bionic_types.end(),
                       bio.info().itype() ) == bionic_types.end() ) {
            bionic_types.push_back( bio.info().itype() );
            if( item::type_is_defined( bio.info().itype() ) ) {
                item tmp = item( bio.id.str(), calendar::turn_zero );
                bionic_names.push_back( tmp.tname() + " - " + format_money( 50000 + ( tmp.price( true ) / 4 ) ) );
            } else {
                bionic_names.push_back( bio.id.str() + " - " + format_money( 50000 ) );
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
        price = 50000 + ( item( bionic_types[bionic_index], calendar::turn_zero ).price( true ) / 4 );
    } else {
        price = 50000;
    }
    if( !npc_trading::pay_npc( p, price ) ) {
        return;
    }

    //Makes the doctor awesome at installing but not perfect
    if( player_character.can_uninstall_bionic( bionic_id( bionic_types[bionic_index].str() ), p,
            false ) ) {
        player_character.amount_of(
            bionic_types[bionic_index] ); // ??? this does nothing, it just queries the count
        player_character.uninstall_bionic( bionic_id( bionic_types[bionic_index].str() ), p, false );
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
    Character &player_character = get_player_character();
    it.set_owner( player_character );
    player_character.i_add( it );
    p.op_of_u.owed -= giving[chosen].price;
    p.add_effect( effect_asked_for_item, 3_hours );
}

void talk_function::give_aid( npc &p )
{
    p.add_effect( effect_currently_busy, 30_minutes );
    Character &player_character = get_player_character();
    for( const bodypart_id &bp :
         player_character.get_all_body_parts( get_body_part_flags::only_main ) ) {
        player_character.heal( bp, 5 * rng( 2, 5 ) );
        if( player_character.has_effect( effect_bite, bp.id() ) ) {
            player_character.remove_effect( effect_bite, bp );
        }
        if( player_character.has_effect( effect_bleed, bp.id() ) ) {
            player_character.remove_effect( effect_bleed, bp );
        }
        if( player_character.has_effect( effect_infected, bp.id() ) ) {
            player_character.remove_effect( effect_infected, bp );
        }
    }
    const int moves = to_moves<int>( 100_minutes );
    player_character.assign_activity( ACT_WAIT_NPC, moves );
    player_character.activity.str_values.push_back( p.name );
}

void talk_function::give_all_aid( npc &p )
{
    p.add_effect( effect_currently_busy, 30_minutes );
    give_aid( p );
    for( npc &guy : g->all_npcs() ) {
        if( guy.is_walking_with() && rl_dist( guy.pos(), get_player_character().pos() ) < PICKUP_RANGE ) {
            for( const bodypart_id &bp :
                 guy.get_all_body_parts( get_body_part_flags::only_main ) ) {
                guy.heal( bp, 5 * rng( 2, 5 ) );
                if( guy.has_effect( effect_bite, bp.id() ) ) {
                    guy.remove_effect( effect_bite, bp );
                }
                if( guy.has_effect( effect_bleed, bp.id() ) ) {
                    guy.remove_effect( effect_bleed, bp );
                }
                if( guy.has_effect( effect_infected, bp.id() ) ) {
                    guy.remove_effect( effect_infected, bp );
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
    Character &player_character = get_player_character();
    trait_id cur_hair;
    for( const trait_id &elem : hair_muts ) {
        if( player_character.has_trait( elem ) ) {
            cur_hair = elem;
        }
        index += 1;
        hair_menu.addentry( index, true, MENU_AUTOASSIGN, elem.obj().name() );
    }
    hair_menu.query();
    int choice = hair_menu.ret;
    if( choice != 0 ) {
        if( player_character.has_trait( cur_hair ) ) {
            player_character.remove_mutation( cur_hair, true );
        }
        player_character.set_mutation( hair_muts[ choice - 1 ] );
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
    Character &player_character = get_player_character();
    player_character.add_morale( MORALE_HAIRCUT, 5, 5, 720_minutes, 3_minutes );
    const int moves = to_moves<int>( 20_minutes );
    player_character.assign_activity( ACT_WAIT_NPC, moves );
    player_character.activity.str_values.push_back( p.name );
    add_msg( m_good, _( "%s gives you a decent haircut…" ), p.name );
}

void talk_function::buy_shave( npc &p )
{
    Character &player_character = get_player_character();
    player_character.add_morale( MORALE_SHAVE, 10, 10, 360_minutes, 3_minutes );
    const int moves = to_moves<int>( 5_minutes );
    player_character.assign_activity( ACT_WAIT_NPC, moves );
    player_character.activity.str_values.push_back( p.name );
    add_msg( m_good, _( "%s gives you a decent shave…" ), p.name );
}

void talk_function::morale_chat( npc &p )
{
    get_player_character().add_morale( MORALE_CHAT, rng( 3, 10 ), 10, 200_minutes, 5_minutes / 2 );
    add_msg( m_good, _( "That was a pleasant conversation with %s…" ), p.disp_name() );
}

void talk_function::morale_chat_activity( npc &p )
{
    Character &player_character = get_player_character();
    const int moves = to_moves<int>( 10_minutes );
    player_character.assign_activity( ACT_SOCIALIZE, moves );
    player_character.activity.str_values.push_back( p.name );
    add_msg( m_good, _( "That was a pleasant conversation with %s." ), p.disp_name() );
    player_character.add_morale( MORALE_CHAT, rng( 3, 10 ), 10, 200_minutes, 5_minutes / 2 );
}

void talk_function::buy_10_logs( npc &p )
{
    std::vector<tripoint_abs_omt> places =
        overmap_buffer.find_all( get_player_character().global_omt_location(), "ranch_camp_67", 1,
                                 false );
    if( places.empty() ) {
        debugmsg( "Couldn't find %s", "ranch_camp_67" );
        return;
    }
    const auto &cur_om = g->get_cur_om();
    std::vector<tripoint_abs_omt> places_om;
    for( const tripoint_abs_omt &i : places ) {
        if( &cur_om == overmap_buffer.get_existing_om_global( i ).om ) {
            places_om.push_back( i );
        }
    }

    const tripoint_abs_omt site = random_entry( places_om );
    tinymap bay;
    bay.load( project_to<coords::sm>( site ), false );
    bay.spawn_item( point( 7, 15 ), "log", 10 );
    bay.save();

    p.add_effect( effect_currently_busy, 1_days );
    add_msg( m_good, _( "%s drops the logs off in the garage…" ), p.name );
}

void talk_function::buy_100_logs( npc &p )
{
    std::vector<tripoint_abs_omt> places =
        overmap_buffer.find_all( get_player_character().global_omt_location(), "ranch_camp_67", 1,
                                 false );
    if( places.empty() ) {
        debugmsg( "Couldn't find %s", "ranch_camp_67" );
        return;
    }
    const auto &cur_om = g->get_cur_om();
    std::vector<tripoint_abs_omt> places_om;
    for( auto &i : places ) {
        if( &cur_om == overmap_buffer.get_existing_om_global( i ).om ) {
            places_om.push_back( i );
        }
    }

    const tripoint_abs_omt site = random_entry( places_om );
    tinymap bay;
    bay.load( project_to<coords::sm>( site ), false );
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
    get_player_character().cash += p.cash;
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

    if( p.sees( get_player_character() ) ) {
        add_msg( _( "%s turns hostile!" ), p.name );
    }

    get_event_bus().send<event_type::npc_becomes_hostile>( p.getID(), p.name );
    p.set_attitude( NPCATT_KILL );
}

void talk_function::flee( npc &p )
{
    add_msg( _( "%s turns to flee!" ), p.name );
    p.set_attitude( NPCATT_FLEE );
}

void talk_function::lightning( npc & )
{
    if( get_player_character().posz() >= 0 ) {
        get_weather().lightning_active = true;
    }
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

bool talk_function::drop_stolen_item( item &cur_item, npc &p )
{
    Character &player_character = get_player_character();
    map &here = get_map();
    bool dropped = false;
    if( cur_item.is_old_owner( p ) ) {
        item to_drop = player_character.i_rem( &cur_item );
        to_drop.remove_old_owner();
        to_drop.set_owner( p );
        here.add_item_or_charges( player_character.pos(), to_drop );
        dropped = true;
    } else if( cur_item.is_container() ) {
        bool changed = false;
        for( item *contained : cur_item.all_items_top() ) {
            changed |= drop_stolen_item( *contained, p );
        }
        if( changed ) {
            dropped = true;
            cur_item.on_contents_changed();
        }
    }
    return dropped;
}

void talk_function::drop_stolen_item( npc &p )
{
    bool dropped = false;
    Character &player_character = get_player_character();
    for( item *&elem : player_character.inv_dump() ) {
        dropped |= drop_stolen_item( *elem, p );
    }
    if( dropped ) {
        player_character.invalidate_weight_carried_cache();
    } else {
        debugmsg( "Failed to drop any stolen items." );
    }
    if( p.known_stolen_item ) {
        p.known_stolen_item = nullptr;
    }
    if( player_character.is_hauling() ) {
        player_character.stop_hauling();
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
    item weap = p.remove_weapon();
    get_map().add_item_or_charges( p.pos(), weap );
}

void talk_function::player_weapon_away( npc &/*p*/ )
{
    Character &player_character = get_player_character();
    player_character.i_add( player_character.remove_weapon() );
}

void talk_function::player_weapon_drop( npc &/*p*/ )
{
    Character &player_character = get_player_character();
    item weap = player_character.remove_weapon();
    get_map().add_item_or_charges( player_character.pos(), weap );
}

void talk_function::lead_to_safety( npc &p )
{
    mission *reach_safety__mission = mission::reserve_new( mission_type_id( "MISSION_REACH_SAFETY" ),
                                     character_id() );
    reach_safety__mission->assign( get_avatar() );
    p.goal = reach_safety__mission->get_target();
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
    const proficiency_id &proficiency = p.chatbin.proficiency;
    int expert_multiplier = 1;
    Character &you = get_player_character();
    if( skill != skill_id() &&
        you.get_skill_level( skill ) < p.get_skill_level( skill ) ) {
        cost = calc_skill_training_cost( p, skill );
        time = calc_skill_training_time( p, skill );
        name = skill.str();
    } else if( p.chatbin.style != matype_id() &&
               !you.martial_arts_data->has_martialart( style ) ) {
        cost = calc_ma_style_training_cost( p, style );
        time = calc_ma_style_training_time( p, style );
        name = p.chatbin.style.str();
        // already checked if can learn this spell in npctalk.cpp
    } else if( p.chatbin.dialogue_spell != spell_id() ) {
        const spell &temp_spell = p.magic->get_spell( sp_id );
        const bool knows = you.magic->knows_spell( sp_id );
        cost = p.calc_spell_training_cost( knows, temp_spell.get_difficulty(),
                                           temp_spell.get_level() );
        name = temp_spell.id().str();
        expert_multiplier = knows ? temp_spell.get_level() -
                            you.magic->get_spell( sp_id ).get_level() : 1;
        // quicker to learn with instruction as opposed to books.
        // if this is a known spell, then there is a set time to gain some exp.
        // if player doesn't know this spell, then the NPC will teach all of it
        // which takes max 6 hours, min 3 hours.
        // TODO: a system for NPCs to train new stuff in bits and pieces
        // and remember the progress.
        if( knows ) {
            time = 1_hours;
        } else {
            const int time_int = you.magic->time_to_learn_spell( you, sp_id ) / 50;
            time = time_duration::from_seconds( clamp( time_int, 7200, 21600 ) );
        }

    } else if( proficiency != proficiency_id() ) {
        name = proficiency.str();
        cost = calc_proficiency_training_cost( p, proficiency );
        time = calc_proficiency_training_time( p, proficiency );
    } else {
        debugmsg( "start_training with no valid skill or style set" );
        return;
    }

    mission *miss = p.chatbin.mission_selected;
    if( miss != nullptr && miss->get_assigned_player_id() == you.getID() &&
        miss->is_complete( you.getID() ) ) {
        clear_mission( p );
    } else if( !npc_trading::pay_npc( p, cost ) ) {
        return;
    }
    player_activity act = player_activity( ACT_TRAIN, to_moves<int>( time ),
                                           p.getID().get_value(), 0, name );
    act.values.push_back( expert_multiplier );
    you.assign_activity( act );

    p.add_effect( effect_asked_to_train, 6_hours );
}

npc *pick_follower()
{
    std::vector<npc *> followers;
    std::vector<tripoint> locations;

    for( npc &guy : g->all_npcs() ) {
        if( guy.is_player_ally() && get_player_view().sees( guy ) ) {
            followers.push_back( &guy );
            locations.push_back( guy.pos() );
        }
    }

    pointmenu_cb callback( locations );

    uilist menu;
    menu.text = _( "Select a follower" );
    menu.callback = &callback;
    menu.w_y_setup = 2;

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
