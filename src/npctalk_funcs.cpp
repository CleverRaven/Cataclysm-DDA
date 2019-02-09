#include "npctalk.h" // IWYU pragma: associated

#include <algorithm>
#include <string>
#include <vector>

#include "basecamp.h"
#include "bionics.h"
#include "debug.h"
#include "game.h"
#include "itype.h"
#include "line.h"
#include "map.h"
#include "messages.h"
#include "mission.h"
#include "morale_types.h"
#include "npc.h"
#include "npctrade.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "requirements.h"
#include "rng.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui.h"
#include "units.h"

#define dbg(x) DebugLog((DebugLevel)(x), D_NPC) << __FILE__ << ":" << __LINE__ << ": "

const efftype_id effect_allow_sleep( "allow_sleep" );
const efftype_id effect_asked_for_item( "asked_for_item" );
const efftype_id effect_asked_personal_info( "asked_personal_info" );
const efftype_id effect_asked_to_follow( "asked_to_follow" );
const efftype_id effect_asked_to_lead( "asked_to_lead" );
const efftype_id effect_asked_to_train( "asked_to_train" );
const efftype_id effect_bite( "bite" );
const efftype_id effect_bleed( "bleed" );
const efftype_id effect_currently_busy( "currently_busy" );
const efftype_id effect_infected( "infected" );
const efftype_id effect_lying_down( "lying_down" );
const efftype_id effect_sleep( "sleep" );

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

    int miss_val = cash_to_favor( p, miss->get_value() );
    npc_opinion tmp( 0, 0, 1 + miss_val / 5, -1, 0 );
    p.op_of_u += tmp;
    if( p.my_fac != nullptr ) {
        int fac_val = std::min( 1 + miss_val / 10, 10 );
        p.my_fac->likes_u += fac_val;
        p.my_fac->respects_u += fac_val;
        p.my_fac->power += fac_val;
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
    trade( p, 0, _( "Reward" ) );
}

void talk_function::start_trade( npc &p )
{
    trade( p, 0, _( "Trade" ) );
}

std::string bulk_trade_inquire( const npc &, const itype_id &it )
{
    int you_have = g->u.charges_of( it );
    item tmp( it );
    int item_cost = tmp.price( true );
    tmp.charges = you_have;
    int total_cost = tmp.price( true );
    return string_format( _( "I'm willing to pay %s per batch for a total of %s" ),
                          format_money( item_cost ), format_money( total_cost ) );
}

void bulk_trade_accept( npc &, const itype_id &it )
{
    int you_have = g->u.charges_of( it );
    item tmp( it );
    tmp.charges = you_have;
    int total = tmp.price( true );
    g->u.use_charges( it, you_have );
    g->u.cash += total;
}

void talk_function::assign_base( npc &p )
{
    // TODO: decide what to do upon assign? maybe pathing required
    basecamp *camp = g->m.camp_at( g->u.pos() );
    if( !camp ) {
        dbg( D_ERROR ) << "talk_function::assign_base: Assigned to base but no base here.";
        return;
    }

    add_msg( _( "%1$s waits at %2$s" ), p.name, camp->camp_name() );
    p.mission = NPC_MISSION_BASE;
    p.set_attitude( NPCATT_NULL );
}

void talk_function::assign_guard( npc &p )
{
    add_msg( _( "%s is posted as a guard." ), p.name );
    p.set_attitude( NPCATT_NULL );
    p.mission = NPC_MISSION_GUARD_ALLY;
    p.chatbin.first_topic = "TALK_FRIEND_GUARD";
    p.set_destination();
}

void talk_function::stop_guard( npc &p )
{
    p.set_attitude( NPCATT_FOLLOW );
    add_msg( _( "%s begins to follow you." ), p.name );
    p.mission = NPC_MISSION_NULL;
    p.chatbin.first_topic = "TALK_FRIEND";
    p.goal = npc::no_goal_point;
    p.guard_pos = npc::no_goal_point;
}

void talk_function::wake_up( npc &p )
{
    p.rules.clear_flag( ally_rule::allow_sleep );
    p.remove_effect( effect_allow_sleep );
    p.remove_effect( effect_lying_down );
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
    std::vector<item *> bionic_inv = g->u.items_with( []( const item & itm ) {
        return itm.is_bionic();
    } );
    if( bionic_inv.empty() ) {
        popup( _( "You have no bionics to install!" ) );
        return;
    }

    std::vector<itype_id> bionic_types;
    std::vector<std::string> bionic_names;
    for( auto &bio : bionic_inv ) {
        if( std::find( bionic_types.begin(), bionic_types.end(), bio->typeId() ) == bionic_types.end() ) {
            if( !g->u.has_bionic( bionic_id( bio->typeId() ) ) || bio->typeId() ==  "bio_power_storage" ||
                bio->typeId() ==  "bio_power_storage_mkII" ) {

                bionic_types.push_back( bio->typeId() );
                bionic_names.push_back( bio->tname() + " - " + format_money( bio->price( true ) * 2 ) );
            }
        }
    }
    // Choose bionic if applicable
    int bionic_index = uilist( _( "Which bionic do you wish to have installed?" ),
                               bionic_names );
    // Did we cancel?
    if( bionic_index < 0 ) {
        popup( _( "You decide to hold off..." ) );
        return;
    }

    const item tmp = item( bionic_types[bionic_index], 0 );
    const itype &it = *tmp.type;
    unsigned int price = tmp.price( true ) * 2;

    if( price > g->u.cash ) {
        popup( _( "You can't afford the procedure..." ) );
        return;
    }

    //Makes the doctor awesome at installing but not perfect
    if( g->u.install_bionics( it, p, false, 20 ) ) {
        g->u.cash -= price;
        p.cash += price;
        g->u.amount_of( bionic_types[bionic_index] );
        std::vector<item_comp> comps;
        comps.push_back( item_comp( tmp.typeId(), 1 ) );
        g->u.consume_items( comps, 1 );
    }
}

void talk_function::bionic_remove( npc &p )
{
    bionic_collection all_bio = *g->u.my_bionics;
    if( all_bio.empty() ) {
        popup( _( "You don't have any bionics installed..." ) );
        return;
    }

    std::vector<itype_id> bionic_types;
    std::vector<std::string> bionic_names;
    for( auto &bio : all_bio ) {
        if( std::find( bionic_types.begin(), bionic_types.end(), bio.id.str() ) == bionic_types.end() ) {
            if( bio.id != bionic_id( "bio_power_storage" ) ||
                bio.id != bionic_id( "bio_power_storage_mkII" ) ) {
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
        popup( _( "You decide to hold off..." ) );
        return;
    }

    unsigned int price;
    if( item::type_is_defined( bionic_types[bionic_index] ) ) {
        price = 50000 + ( item( bionic_types[bionic_index], 0 ).price( true ) / 4 );
    } else {
        price = 50000;
    }
    if( price > g->u.cash ) {
        popup( _( "You can't afford the procedure..." ) );
        return;
    }

    //Makes the doctor awesome at installing but not perfect
    if( g->u.uninstall_bionic( bionic_id( bionic_types[bionic_index] ), p, false ) ) {
        g->u.cash -= price;
        p.cash += price;
        g->u.amount_of( bionic_types[bionic_index] ); // ??? this does nothing, it just queries the count
    }

}

void talk_function::give_equipment( npc &p )
{
    std::vector<item_pricing> giving = init_selling( p );
    int chosen = -1;
    while( chosen == -1 && giving.size() > 1 ) {
        int index = rng( 0, giving.size() - 1 );
        if( giving[index].price < p.op_of_u.owed ) {
            chosen = index;
        }
        giving.erase( giving.begin() + index );
    }
    if( giving.empty() ) {
        popup( _( "%s has nothing to give!" ), p.name );
        return;
    }
    if( chosen == -1 ) {
        chosen = 0;
    }
    item it = *giving[chosen].loc.get_item();
    giving[chosen].loc.remove_item();
    popup( _( "%1$s gives you a %2$s" ), p.name, it.tname() );

    g->u.i_add( it );
    p.op_of_u.owed -= giving[chosen].price;
    p.add_effect( effect_asked_for_item, 3_hours );
}

void talk_function::give_aid( npc &p )
{
    p.add_effect( effect_currently_busy, 30_minutes );
    for( int i = 0; i < num_hp_parts; i++ ) {
        const body_part bp_healed = player::hp_to_bp( hp_part( i ) );
        g->u.heal( hp_part( i ), 5 * rng( 2, 5 ) );
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
    g->u.assign_activity( activity_id( "ACT_WAIT_NPC" ), 10000 );
    g->u.activity.str_values.push_back( p.name );
}

void talk_function::give_all_aid( npc &p )
{
    p.add_effect( effect_currently_busy, 30_minutes );
    give_aid( p );
    for( npc &guy : g->all_npcs() ) {
        if( rl_dist( guy.pos(), g->u.pos() ) < PICKUP_RANGE && guy.is_friend() ) {
            for( int i = 0; i < num_hp_parts; i++ ) {
                const body_part bp_healed = player::hp_to_bp( hp_part( i ) );
                guy.heal( hp_part( i ), 5 * rng( 2, 5 ) );
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

void talk_function::buy_haircut( npc &p )
{
    g->u.add_morale( MORALE_HAIRCUT, 5, 5, 720_minutes, 3_minutes );
    g->u.assign_activity( activity_id( "ACT_WAIT_NPC" ), 300 );
    g->u.activity.str_values.push_back( p.name );
    add_msg( m_good, _( "%s gives you a decent haircut..." ), p.name );
}

void talk_function::buy_shave( npc &p )
{
    g->u.add_morale( MORALE_SHAVE, 10, 10, 360_minutes, 3_minutes );
    g->u.assign_activity( activity_id( "ACT_WAIT_NPC" ), 100 );
    g->u.activity.str_values.push_back( p.name );
    add_msg( m_good, _( "%s gives you a decent shave..." ), p.name );
}

void talk_function::morale_chat( npc &p )
{
    g->u.add_morale( MORALE_CHAT, rng( 3, 10 ), 10, 200_minutes, 5_minutes / 2 );
    add_msg( m_good, _( "That was a pleasant conversation with %s..." ), p.disp_name() );
}

void talk_function::morale_chat_activity( npc &p )
{
    g->u.assign_activity( activity_id( "ACT_SOCIALIZE" ), 10000 );
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
        if( &cur_om == overmap_buffer.get_existing_om_global( i ) ) {
            places_om.push_back( i );
        }
    }

    const tripoint site = random_entry( places_om );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.spawn_item( 7, 15, "log", 10 );
    bay.save();

    p.add_effect( effect_currently_busy, 1_days );
    add_msg( m_good, _( "%s drops the logs off in the garage..." ), p.name );
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
        if( &cur_om == overmap_buffer.get_existing_om_global( i ) ) {
            places_om.push_back( i );
        }
    }

    const tripoint site = random_entry( places_om );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.spawn_item( 7, 15, "log", 100 );
    bay.save();

    p.add_effect( effect_currently_busy, 7_days );
    add_msg( m_good, _( "%s drops the logs off in the garage..." ), p.name );
}

void talk_function::follow( npc &p )
{
    p.set_attitude( NPCATT_FOLLOW );
    g->u.cash += p.cash;
    p.cash = 0;
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

    g->u.add_memorial_log( pgettext( "memorial_male", "%s became hostile." ),
                           pgettext( "memorial_female", "%s became hostile." ),
                           p.name.c_str() );
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
    p.set_attitude( NPCATT_NULL );
}

void talk_function::stranger_neutral( npc &p )
{
    add_msg( _( "%s feels less threatened by you." ), p.name );
    p.set_attitude( NPCATT_NULL );
    p.chatbin.first_topic = "TALK_STRANGER_NEUTRAL";
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
    g->m.add_item_or_charges( p.pos(), p.remove_weapon() );
}

void talk_function::player_weapon_away( npc &p )
{
    ( void )p; //unused
    g->u.i_add( g->u.remove_weapon() );
}

void talk_function::player_weapon_drop( npc &p )
{
    ( void )p; // unused
    g->m.add_item_or_charges( g->u.pos(), g->u.remove_weapon() );
}

void talk_function::lead_to_safety( npc &p )
{
    const auto mission = mission::reserve_new( mission_type_id( "MISSION_REACH_SAFETY" ), -1 );
    mission->assign( g->u );
    p.goal = mission->get_target();
    p.set_attitude( NPCATT_LEAD );
}

bool pay_npc( npc &np, int cost )
{
    if( np.op_of_u.owed >= cost ) {
        np.op_of_u.owed -= cost;
        return true;
    }

    if( g->u.cash + static_cast<unsigned long>( np.op_of_u.owed ) >= static_cast<unsigned long>
        ( cost ) ) {
        g->u.cash -= cost - np.op_of_u.owed;
        np.op_of_u.owed = 0;
        return true;
    }

    return trade( np, -cost, _( "Pay:" ) );
}

void talk_function::start_training( npc &p )
{
    int cost;
    time_duration time = 0_turns;
    std::string name;
    const skill_id &skill = p.chatbin.skill;
    const matype_id &style = p.chatbin.style;
    if( skill.is_valid() && g->u.get_skill_level( skill ) < p.get_skill_level( skill ) ) {
        cost = calc_skill_training_cost( p, skill );
        time = calc_skill_training_time( p, skill );
        name = skill.str();
    } else if( p.chatbin.style.is_valid() && !g->u.has_martialart( style ) ) {
        cost = calc_ma_style_training_cost( p, style );
        time = calc_ma_style_training_time( p, style );
        name = p.chatbin.style.str();
    } else {
        debugmsg( "start_training with no valid skill or style set" );
        return;
    }

    mission *miss = p.chatbin.mission_selected;
    if( miss != nullptr && miss->get_assigned_player_id() == g->u.getID() ) {
        clear_mission( p );
    } else if( !pay_npc( p, cost ) ) {
        return;
    }
    g->u.assign_activity( activity_id( "ACT_TRAIN" ), to_moves<int>( time ), p.getID(), 0, name );
    p.add_effect( effect_asked_to_train, 6_hours );
}

npc *pick_follower()
{
    std::vector<npc *> followers;
    std::vector<tripoint> locations;

    for( npc &guy : g->all_npcs() ) {
        if( guy.is_following() && g->u.sees( guy ) ) {
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
    const std::string title = string_format( _( "Pickup rules for %s" ), p.name );
    p.rules.pickup_whitelist->show( title, false );
}
