#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>

#include "auto_pickup.h"
#include "avatar.h"
#include "calendar.h"
#include "character.h"
#include "character_id.h"
#include "coordinates.h"
#include "debug.h"
#include "dialogue_chatbin.h"
#include "game.h"
#include "game_inventory.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "magic.h"
#include "make_static.h"
#include "martialarts.h"
#include "messages.h"
#include "mission.h"
#include "mission_companion.h"
#include "npc.h"
#include "npctalk.h"
#include "npctrade.h"
#include "output.h"
#include "pimpl.h"
#include "player_activity.h"
#include "proficiency.h"
#include "ret_val.h"
#include "skill.h"
#include "string_formatter.h"
#include "talker.h"
#include "talker_npc.h"
#include "translations.h"
#include "units.h"
#include "units_utility.h"
#include "value_ptr.h"

static const efftype_id effect_lying_down( "lying_down" );
static const efftype_id effect_narcosis( "narcosis" );
static const efftype_id effect_npc_suspend( "npc_suspend" );
static const efftype_id effect_sleep( "sleep" );

static const faction_id faction_no_faction( "no_faction" );

static const itype_id itype_foodperson_mask( "foodperson_mask" );
static const itype_id itype_foodperson_mask_on( "foodperson_mask_on" );

static const trait_id trait_DEBUG_MIND_CONTROL( "DEBUG_MIND_CONTROL" );
static const trait_id trait_PROF_CHURL( "PROF_CHURL" );
static const trait_id trait_PROF_FOODP( "PROF_FOODP" );
static const trait_id trait_SAPROVORE( "SAPROVORE" );

talker_npc::talker_npc( npc *new_me )
{
    me_npc = new_me;
    me_chr = new_me;
    me_chr_const = new_me;
}

std::string talker_npc::distance_to_goal() const
{
    // TODO: this ignores the z-component
    int dist = rl_dist( me_npc->global_omt_location(), me_npc->goal );
    std::string response;
    dist *= 100;
    if( dist >= 1300 ) {
        int miles = dist / 25; // *100, e.g. quarter mile is "25"
        miles -= miles % 25; // Round to nearest quarter-mile
        int fullmiles = ( miles - miles % 100 ) / 100; // Left of the decimal point
        if( fullmiles < 0 ) {
            fullmiles = 0;
        }
        response = string_format( _( "%d.%d miles." ), fullmiles, miles );
    } else {
        response = string_format( n_gettext( "%d foot.", "%d feet.", dist ), dist );
    }
    return response;
}

bool talker_npc::will_talk_to_u( const Character &you, bool force )
{
    if( you.is_dead_state() ) {
        me_npc->set_attitude( NPCATT_NULL );
        return false;
    }
    if( get_player_character().getID() == you.getID() ) {
        if( me_npc->get_faction() ) {
            me_npc->get_faction()->known_by_u = true;
        }
        me_npc->set_known_to_u( true );
    }
    // This is necessary so that we don't bug the player over and over
    if( me_npc->get_attitude() == NPCATT_TALK ) {
        me_npc->set_attitude( NPCATT_NULL );
    } else if( !force && ( me_npc->get_attitude() == NPCATT_FLEE ||
                           me_npc-> get_attitude() == NPCATT_FLEE_TEMP ) ) {
        add_msg( _( "%s is fleeing from you!" ), disp_name() );
        return false;
    } else if( !force && me_npc->get_attitude() == NPCATT_KILL ) {
        add_msg( _( "%s is hostile!" ), disp_name() );
        return false;
    }
    return true;
}

std::vector<std::string> talker_npc::get_topics( bool radio_contact )
{
    avatar &player_character = get_avatar();
    std::vector<std::string> add_topics;
    // For each active mission we have, let the mission know we talked to this NPC.
    for( mission *&mission : player_character.get_active_missions() ) {
        mission->on_talk_with_npc( me_npc->getID() );
    }

    add_topics.push_back( me_npc->chatbin.first_topic );
    if( radio_contact ) {
        add_topics.emplace_back( me_npc->chatbin.talk_radio );
    } else if( me_npc->is_leader() ) {
        add_topics.emplace_back( me_npc->chatbin.talk_leader );
    } else if( me_npc->is_player_ally() && ( me_npc->is_walking_with() || me_npc->has_activity() ) ) {
        add_topics.emplace_back( me_npc->chatbin.talk_friend );
    } else if( me_npc->get_attitude() == NPCATT_RECOVER_GOODS ) {
        add_topics.emplace_back( me_npc->chatbin.talk_stole_item );
    }
    int most_difficult_mission = 0;
    for( mission *&mission : me_npc->chatbin.missions ) {
        const mission_type &type = mission->get_type();
        if( type.urgent && type.difficulty > most_difficult_mission ) {
            add_topics.emplace_back( "TALK_MISSION_DESCRIBE_URGENT" );
            me_npc->chatbin.mission_selected = mission;
            most_difficult_mission = type.difficulty;
        }
    }
    most_difficult_mission = 0;
    bool chosen_urgent = false;
    for( mission *&mission : me_npc->chatbin.missions_assigned ) {
        if( mission->get_assigned_player_id() != player_character.getID() ) {
            // Not assigned to the player that is currently talking to the npc
            continue;
        }
        const mission_type &type = mission->get_type();
        if( ( type.urgent && !chosen_urgent ) || ( type.difficulty > most_difficult_mission &&
                ( type.urgent || !chosen_urgent ) ) ) {
            chosen_urgent = type.urgent;
            add_topics.emplace_back( "TALK_MISSION_INQUIRE" );
            me_npc->chatbin.mission_selected = mission;
            most_difficult_mission = type.difficulty;
        }
    }

    // Needs
    if( me_npc->has_effect( effect_npc_suspend ) ) {
        add_topics.emplace_back( "TALK_REBOOT" );
    }
    if( me_npc->has_effect( effect_sleep ) || me_npc->has_effect( effect_lying_down ) ) {
        if( me_npc->has_effect( effect_narcosis ) ) {
            add_topics.emplace_back( "TALK_SEDATED" );
        } else {
            add_topics.emplace_back( me_npc->chatbin.talk_wake_up );
        }
    }

    if( add_topics.back() == "TALK_NONE" ) {
        add_topics.back() = me_npc->pick_talk_topic( player_character );
    }
    me_npc->moves -= 100;

    if( player_character.is_deaf() ) {
        if( add_topics.back() == me_npc->chatbin.talk_mug ||
            add_topics.back() == me_npc->chatbin.talk_stranger_aggressive ) {
            me_npc->make_angry();
            add_topics.emplace_back( "TALK_DEAF_ANGRY" );
        } else {
            add_topics.emplace_back( "TALK_DEAF" );
        }
    }
    if( player_character.is_mute() ) {
        if( add_topics.back() == me_npc->chatbin.talk_mug ||
            add_topics.back() == me_npc->chatbin.talk_stranger_aggressive ) {
            me_npc->make_angry();
            add_topics.emplace_back( "TALK_MUTE_ANGRY" );
        } else {
            add_topics.emplace_back( "TALK_MUTE" );
        }
    }
    if( player_character.has_trait( trait_PROF_CHURL ) ) {
        if( add_topics.back() == me_npc->chatbin.talk_mug ||
            add_topics.back() == me_npc->chatbin.talk_stranger_aggressive ) {
            me_npc->make_angry();
            add_topics.emplace_back( "TALK_CHURL_ANGRY" );
        } else if( ( me_npc->op_of_u.trust >= 0 ) && ( me_npc->op_of_u.anger <= 0 ) &&
                   ( me_npc->int_cur >= 9 ) ) {
            add_topics.emplace_back( "TALK_CHURL_TRADE" );
        } else {
            add_topics.emplace_back( "TALK_CHURL" );
        }
    }

    if( me_npc->has_trait( trait_PROF_FOODP ) &&
        !( me_npc->is_wearing( itype_foodperson_mask_on ) ||
           me_npc->is_wearing( itype_foodperson_mask ) ) ) {
        add_topics.emplace_back( "TALK_NPC_NOFACE" );
    }
    me_npc->decide_needs();

    return add_topics;
}

void talker_npc::check_missions()
{
    me_npc->chatbin.check_missions();
}

void talker_npc::update_missions( const std::vector<mission *> &missions_assigned )
{
    if( me_npc->chatbin.mission_selected == nullptr ) {
        // if possible, select a mission to talk about
        if( !me_npc->chatbin.missions.empty() ) {
            me_npc->chatbin.mission_selected = me_npc->chatbin.missions.front();
        } else if( !missions_assigned.empty() ) {
            me_npc->chatbin.mission_selected = missions_assigned.front();
        }
    }
}

bool talker_npc::check_hostile_response( const int anger ) const
{
    return me_npc->op_of_u.anger + anger > me_npc->hostile_anger_level();
}

// Every OWED_VAL that the NPC owes you counts as +1 towards convincing
static constexpr int OWED_VAL = 1000;
int talker_npc::parse_mod( const std::string &attribute, const int factor ) const
{
    int modifier = 0;
    if( attribute == "ANGER" ) {
        modifier = me_npc->op_of_u.anger;
    } else if( attribute == "FEAR" ) {
        modifier = me_npc->op_of_u.fear;
    } else if( attribute == "TRUST" ) {
        modifier = me_npc->op_of_u.trust;
    } else if( attribute == "VALUE" ) {
        modifier = me_npc->op_of_u.value;
    } else if( attribute == "POS_FEAR" ) {
        modifier = std::max( 0, me_npc->op_of_u.fear );
    } else if( attribute == "AGGRESSION" ) {
        // NOLINTNEXTLINE(bugprone-signed-char-misuse,cert-str34-c)
        modifier = me_npc->personality.aggression;
    } else if( attribute == "ALTRUISM" ) {
        // NOLINTNEXTLINE(bugprone-signed-char-misuse,cert-str34-c)
        modifier = me_npc->personality.altruism;
    } else if( attribute == "BRAVERY" ) {
        // NOLINTNEXTLINE(bugprone-signed-char-misuse,cert-str34-c)
        modifier = me_npc->personality.bravery;
    } else if( attribute == "COLLECTOR" ) {
        // NOLINTNEXTLINE(bugprone-signed-char-misuse,cert-str34-c)
        modifier = me_npc->personality.collector;
    } else if( attribute == "MISSIONS" ) {
        modifier = me_npc->assigned_missions_value() / OWED_VAL;
    } else if( attribute == "NPC_INTIMIDATE" ) {
        modifier = me_npc->intimidation();
    }
    modifier *= factor;
    return modifier;
}

int talker_npc::trial_chance_mod( const std::string &trial_type ) const
{
    int chance = 0;
    if( trial_type == "lie" ) {
        chance += - me_npc->lie_skill() + me_npc->op_of_u.trust * 3;
    } else if( trial_type == "persuade" ) {
        chance += - static_cast<int>( me_npc->persuade_skill() * 0.5 ) +
                  me_npc->op_of_u.trust * 2 + me_npc->op_of_u.value;
    } else if( trial_type == "intimidate" ) {
        chance += - me_npc->intimidation() + me_npc->op_of_u.fear * 2 -
                  me_npc->personality.bravery * 2;
    }
    return chance;
}

void talker_npc::store_chosen_training( const skill_id &c_skill, const matype_id &c_style,
                                        const spell_id &c_spell, const proficiency_id &c_proficiency )
{
    me_npc->chatbin.store_chosen_training( c_skill, c_style, c_spell, c_proficiency );
}

int talker_npc::debt() const
{
    return me_npc->op_of_u.owed;
}

void talker_npc::add_debt( const int cost )
{
    me_npc->op_of_u.owed += cost;
}

int talker_npc::sold() const
{
    return me_npc->op_of_u.sold;
}

void talker_npc::add_sold( const int value )
{
    me_npc->op_of_u.sold += value;
}

int talker_npc::cash_to_favor( const int value ) const
{
    return npc_trading::cash_to_favor( *me_npc, value );
}

int talker_npc::value( const item &it ) const
{
    return me_npc->value( it );
}

enum consumption_result {
    REFUSED = 0,
    CONSUMED_SOME, // Consumption didn't fail, but don't delete the item
    CONSUMED_ALL   // Consumption succeeded, delete the item
};

// Returns true if we destroyed the item through consumption
// does not try to consume contents
static consumption_result try_consume( npc &p, item &it, std::string &reason )
{
    // TODO: Unify this with 'player::consume_item()'
    item &to_eat = it;
    if( to_eat.is_null() ) {
        debugmsg( "Null item to try_consume." );
        return REFUSED;
    }
    const auto &comest = to_eat.get_comestible();
    if( !comest ) {
        // Don't inform the player that we don't want to eat the lighter
        return REFUSED;
    }

    if( !p.will_accept_from_player( it ) ) {
        reason = _( p.chatbin.snip_consume_cant_accept );
        return REFUSED;
    }

    // TODO: Make it not a copy+paste from player::consume_item
    int amount_used = 1;
    if( to_eat.is_food() ) {
        if( !p.can_consume_as_is( to_eat ) ) {
            reason = _( p.chatbin.snip_consume_cant_consume );
            return REFUSED;
        } else {
            if( to_eat.rotten() && !p.as_character()->has_trait( trait_SAPROVORE ) ) {
                //TODO: once npc needs are operational again check npc hunger state and allow eating if desperate
                reason = _( p.chatbin.snip_consume_rotten );
                return REFUSED;
            }

            const time_duration &consume_time = p.get_consume_time( to_eat );
            p.moves -= to_moves<int>( consume_time );
            p.consume( to_eat );
            reason = _( p.chatbin.snip_consume_eat );
        }

    } else if( to_eat.is_medication() ) {
        if( !comest->tool.is_null() ) {
            bool has = p.has_amount( comest->tool, 1 );
            if( item::count_by_charges( comest->tool ) ) {
                has = p.has_charges( comest->tool, 1 );
            }
            if( !has ) {
                std::string talktag = p.chatbin.snip_consume_need_item;
                parse_tags( talktag, get_player_character(), p );
                reason = string_format( _( talktag ), item::nname( comest->tool ) );
                return REFUSED;
            }
            p.use_charges( comest->tool, 1 );
            reason = _( p.chatbin.snip_consume_med );
        }
        if( to_eat.type->has_use() ) {
            amount_used = to_eat.type->invoke( &p, to_eat, p.pos() ).value_or( 0 );
            if( amount_used <= 0 ) {
                reason = _( p.chatbin.snip_consume_nocharge );
                return REFUSED;
            }
            reason = _( p.chatbin.snip_consume_use_med );
        }

        p.consume_effects( to_eat );
        to_eat.charges -= amount_used;
        p.moves -= 250;
    } else {
        debugmsg( "Unknown comestible type of item: %s\n", to_eat.tname() );
    }

    if( to_eat.charges > 0 ) {
        return CONSUMED_SOME;
    }

    // If not consuming contents and charge <= 0, we just ate the last charge from the stack
    return CONSUMED_ALL;
}

std::string talker_npc::give_item_to( const bool to_use )
{
    avatar &player_character = get_avatar();
    if( me_npc->is_hallucination() ) {
        return _( me_npc->chatbin.snip_give_to_hallucination );
    }
    item_location loc = game_menus::inv::titled_menu( player_character, _( "Offer what?" ),
                        _( "You have no items to offer." ) );
    if( !loc ) {
        return _( me_npc->chatbin.snip_give_cancel );
    }
    item &given = *loc;

    if( ( loc == player_character.get_wielded_item() &&
          given.has_flag( STATIC( flag_id( "NO_UNWIELD" ) ) ) ) ||
        ( player_character.is_worn( given ) &&
          ( given.has_flag( STATIC( flag_id( "NO_TAKEOFF" ) ) ) ||
            given.has_flag( STATIC( flag_id( "INTEGRATED" ) ) ) ) ) ) {
        // Integrated item or shackles
        return _( "How?" );
    }

    if( given.is_dangerous() && !player_character.has_trait( trait_DEBUG_MIND_CONTROL ) ) {
        return _( me_npc->chatbin.snip_give_dangerous );
    }

    bool taken = false;
    std::string reason = _( me_npc->chatbin.snip_give_nope );
    const item_location weapon = me_npc->get_wielded_item();
    int our_ammo = me_npc->ammo_count_for( weapon );
    int new_ammo = me_npc->ammo_count_for( loc );
    const double new_weapon_value = me_npc->weapon_value( given, new_ammo );
    const item &weap = weapon ? *weapon : null_item_reference();
    const double cur_weapon_value = me_npc->weapon_value( weap, our_ammo );
    add_msg_debug( debugmode::DF_TALKER, "NPC evaluates own %s (%d ammo): %0.1f",
                   weap.typeId().str(), our_ammo, cur_weapon_value );
    add_msg_debug( debugmode::DF_TALKER, "NPC evaluates your %s (%d ammo): %0.1f",
                   given.typeId().str(), new_ammo, new_weapon_value );
    if( to_use ) {
        // Eating first, to avoid evaluating bread as a weapon
        const consumption_result consume_res = try_consume( *me_npc, given, reason );
        if( consume_res != REFUSED ) {
            player_character.moves -= 100;
            if( consume_res == CONSUMED_ALL ) {
                player_character.i_rem( &given );
            } else if( given.is_container() ) {
                given.on_contents_changed();
            }
        }// wield it if its a weapon
        else if( new_weapon_value > cur_weapon_value ) {
            me_npc->wield( given );
            reason = _( me_npc->chatbin.snip_give_wield );
            taken = true;
        }// HACK: is_gun here is a hack to prevent NPCs wearing guns if they don't want to use them
        else if( !given.is_gun() && given.is_armor() ) {
            //if it is impossible to wear return why
            ret_val<void> can_wear = me_npc->can_wear( given, true );
            if( !can_wear.success() ) {
                reason = can_wear.str();
            } else {
                //if we can wear it with equip changes prompt first
                can_wear = me_npc->can_wear( given );
                if( ( can_wear.success() ||
                      query_yn( can_wear.str() + _( " Should I take something off?" ) ) )
                    && me_npc->wear_if_wanted( given, reason ) ) {
                    taken = true;
                } else {
                    reason = can_wear.str();
                }
            }
        } else {
            reason += " " + string_format( _( me_npc->chatbin.snip_give_weapon_weak +
                                              "(new weapon value: %.1f vs %.1f)." ), new_weapon_value, cur_weapon_value );
        }
    } else {//allow_use is false so try to carry instead
        if( me_npc->can_pickVolume( given ) && me_npc->can_pickWeight( given ) ) {
            reason = _( me_npc->chatbin.snip_give_carry );
            // set the item given to be favorited so it's not dropped automatically
            given.set_favorite( true );
            taken = true;
            me_npc->i_add( given );
        } else {
            if( !me_npc->can_pickVolume( given ) ) {
                const units::volume free_space = me_npc->volume_capacity() -
                                                 me_npc->volume_carried();
                reason += " " + std::string( _( me_npc->chatbin.snip_give_carry_cant ) ) + " ";
                if( free_space > 0_ml ) {
                    std::string talktag = me_npc->chatbin.snip_give_carry_cant_few_space;
                    parse_tags( talktag, get_player_character(), *me_npc );
                    reason += string_format( _( talktag ), format_volume( free_space ), volume_units_long() );
                } else {
                    reason += _( me_npc->chatbin.snip_give_carry_cant_no_space );
                }
            }
            if( !me_npc->can_pickWeight( given ) ) {
                reason += std::string( " " ) + _( me_npc->chatbin.snip_give_carry_too_heavy );
            }
        }
    }

    if( taken ) {
        player_character.i_rem( &given );
        player_character.moves -= 100;
        me_npc->has_new_items = true;
    }

    return reason;
}

bool talker_npc::buy_from( const int amount )
{
    return npc_trading::pay_npc( *me_npc, amount );
}

std::vector<mission *> talker_npc::available_missions() const
{
    return me_npc->chatbin.missions;
}

std::vector<mission *> talker_npc::assigned_missions() const
{
    return me_npc->chatbin.missions_assigned;
}

mission *talker_npc::selected_mission() const
{
    return me_npc->chatbin.mission_selected;
}

void talker_npc::select_mission( mission *selected )
{
    me_npc->chatbin.mission_selected = selected;
}

void talker_npc::add_mission( const mission_type_id &mission_id )
{
    mission *miss = mission::reserve_new( mission_id, me_npc->getID() );
    miss->assign( get_avatar() );
    me_npc->chatbin.missions_assigned.push_back( miss );
}

void talker_npc::set_companion_mission( const std::string &role_id )
{
    me_npc->companion_mission_role_id = role_id;
    talk_function::companion_mission( *me_npc );
}

void talker_npc::set_fac( const faction_id &new_fac_name )
{
    me_npc->set_fac( new_fac_name );
}

void talker_npc::add_faction_rep( const int rep_change )
{
    if( me_npc->get_faction()-> id != faction_no_faction ) {
        me_npc->get_faction()->likes_u += rep_change;
        me_npc->get_faction()->respects_u += rep_change;
        me_npc->get_faction()->trusts_u += rep_change;
    }
}

bool talker_npc::is_following() const
{
    return me_npc->is_following();
}

bool talker_npc::is_friendly( const Character &guy ) const
{
    return me_npc->is_friendly( guy );
}

bool talker_npc::is_enemy() const
{
    return me_npc->is_enemy();
}

bool talker_npc::is_player_ally()  const
{
    return me_npc->is_player_ally();
}

bool talker_npc::turned_hostile() const
{
    return me_npc->turned_hostile();
}

void talker_npc::make_angry()
{
    me_npc->make_angry();
}

bool talker_npc::has_ai_rule( const std::string &type,
                              const std::string &rule ) const
{
    if( type == "aim_rule" ) {
        auto rule_val = aim_rule_strs.find( rule );
        if( rule_val != aim_rule_strs.end() ) {
            return me_npc->rules.aim == rule_val->second;
        }
    } else if( type == "engagement_rule" ) {
        auto rule_val = combat_engagement_strs.find( rule );
        if( rule_val != combat_engagement_strs.end() ) {
            return me_npc->rules.engagement == rule_val->second;
        }
    } else if( type == "cbm_reserve_rule" ) {
        auto rule_val = cbm_reserve_strs.find( rule );
        if( rule_val != cbm_reserve_strs.end() ) {
            return me_npc->rules.cbm_reserve == rule_val->second;
        }
    } else if( type == "cbm_recharge_rule" ) {
        auto rule_val = cbm_recharge_strs.find( rule );
        if( rule_val != cbm_recharge_strs.end() ) {
            return me_npc->rules.cbm_recharge == rule_val->second;
        }
    } else if( type == "ally_rule" ) {
        auto rule_val = ally_rule_strs.find( rule );
        if( rule_val != ally_rule_strs.end() ) {
            return me_npc->rules.has_flag( rule_val->second.rule );
        }
    } else if( type == "ally_override" ) {
        auto rule_val = ally_rule_strs.find( rule );
        if( rule_val != ally_rule_strs.end() ) {
            return me_npc->rules.has_override_enable( rule_val->second.rule );
        }
    } else if( type == "pickup_rule" ) {
        if( rule == "any" ) {
            return !me_npc->rules.pickup_whitelist->empty();
        }
    }

    return false;
}

void talker_npc::toggle_ai_rule( const std::string &, const std::string &rule )
{
    auto toggle = ally_rule_strs.find( rule );
    if( toggle == ally_rule_strs.end() ) {
        return;
    }
    me_npc->rules.toggle_flag( toggle->second.rule );
    me_npc->invalidate_range_cache();
    me_npc->wield_better_weapon();
}

void talker_npc::set_ai_rule( const std::string &type, const std::string &rule )
{
    if( type == "aim_rule" ) {
        auto rule_val = aim_rule_strs.find( rule );
        if( rule_val != aim_rule_strs.end() ) {
            me_npc->rules.aim = rule_val->second;
            me_npc->invalidate_range_cache();
        }
    } else if( type == "engagement_rule" ) {
        auto rule_val = combat_engagement_strs.find( rule );
        if( rule_val != combat_engagement_strs.end() ) {
            me_npc->rules.engagement = rule_val->second;
            me_npc->invalidate_range_cache();
            me_npc->wield_better_weapon();
        }
    } else if( type == "cbm_reserve_rule" ) {
        auto rule_val = cbm_reserve_strs.find( rule );
        if( rule_val != cbm_reserve_strs.end() ) {
            me_npc->rules.cbm_reserve = rule_val->second;
        }
    } else if( type == "cbm_recharge_rule" ) {
        auto rule_val = cbm_recharge_strs.find( rule );
        if( rule_val != cbm_recharge_strs.end() ) {
            me_npc->rules.cbm_recharge = rule_val->second;
        }
    } else if( type == "ally_rule" ) {
        auto toggle = ally_rule_strs.find( rule );
        if( toggle == ally_rule_strs.end() ) {
            return;
        }
        me_npc->rules.set_flag( toggle->second.rule );
        me_npc->invalidate_range_cache();
        me_npc->wield_better_weapon();
    }
}

void talker_npc::clear_ai_rule( const std::string &, const std::string &rule )
{
    auto toggle = ally_rule_strs.find( rule );
    if( toggle == ally_rule_strs.end() ) {
        return;
    }
    me_npc->rules.clear_flag( toggle->second.rule );
    me_npc->invalidate_range_cache();
    me_npc->wield_better_weapon();
}

std::string talker_npc::get_job_description() const
{
    return me_npc->describe_mission();
}

std::string talker_npc::evaluation_by( const talker &alpha ) const
{
    if( !alpha.can_see() ) {
        return _( "&You're blind and can't make anything out." );
    }

    ///\EFFECT_PER affects whether player can size up NPCs

    ///\EFFECT_INT slightly affects whether player can size up NPCs
    int ability = alpha.per_cur() * 3 + alpha.int_cur();
    if( ability <= 10 ) {
        return _( "&You can't make anything out." );
    }

    if( is_player_ally() || ability > 100 ) {
        ability = 100;
    }

    std::string info = "&";
    int str_range = static_cast<int>( 100 / ability );
    int str_min = static_cast<int>( me_npc->str_max / str_range ) * str_range;
    info += string_format( _( "Str %d - %d" ), str_min, str_min + str_range );

    if( ability >= 40 ) {
        int dex_range = static_cast<int>( 160 / ability );
        int dex_min = static_cast<int>( me_npc->dex_max / dex_range ) * dex_range;
        info += string_format( _( "  Dex %d - %d" ), dex_min, dex_min + dex_range );
    }

    if( ability >= 50 ) {
        int int_range = static_cast<int>( 200 / ability );
        int int_min = static_cast<int>( me_npc->int_max / int_range ) * int_range;
        info += string_format( _( "  Int %d - %d" ), int_min, int_min + int_range );
    }

    if( ability >= 60 ) {
        int per_range = static_cast<int>( 240 / ability );
        int per_min = static_cast<int>( me_npc->per_max / per_range ) * per_range;
        info += string_format( _( "  Per %d - %d" ), per_min, per_min + per_range );
    }
    needs_rates rates = me_npc->calc_needs_rates();
    if( ability >= 100 - ( get_fatigue() / 10 ) ) {
        std::string how_tired;
        if( get_fatigue() > fatigue_levels::EXHAUSTED ) {
            how_tired = _( "Exhausted" );
        } else if( get_fatigue() > fatigue_levels::DEAD_TIRED ) {
            how_tired = _( "Dead tired" );
        } else if( get_fatigue() > fatigue_levels::TIRED ) {
            how_tired = _( "Tired" );
        } else {
            how_tired = _( "Not tired" );
            if( ability >= 100 ) {
                time_duration sleep_at = 5_minutes * ( fatigue_levels::TIRED -
                                                       get_fatigue() ) / rates.fatigue;
                how_tired += _( ".  Will need sleep in " ) + to_string_approx( sleep_at );
            }
        }
        info += "\n" + how_tired;
    }
    if( ability >= 100 ) {
        if( get_thirst() < 100 ) {
            time_duration thirst_at = 5_minutes * ( 100 - get_thirst() ) / rates.thirst;
            if( thirst_at > 1_hours ) {
                info += _( "\nWill need water in " ) + to_string_approx( thirst_at );
            }
        } else {
            info += _( "\nThirsty" );
        }
        if( get_hunger() < 100 ) {
            time_duration hunger_at = 5_minutes * ( 100 - get_hunger() ) / rates.hunger;
            if( hunger_at > 1_hours ) {
                info += _( "\nWill need food in " ) + to_string_approx( hunger_at );
            }
        } else {
            info += _( "\nHungry" );
        }
    }
    return info;

}

bool talker_npc::has_activity() const
{
    return !me_npc->activity.is_null();
}

bool talker_npc::is_myclass( const npc_class_id &class_to_check ) const
{
    return me_npc->myclass == class_to_check;
}

void talker_npc::set_class( const npc_class_id &new_class )
{
    me_npc->myclass = new_class;
}

void talker_npc::say( const std::string &speech )
{
    me_npc->say( speech );
}

std::string talker_npc::opinion_text() const
{
    return me_npc->opinion_text();
}

void talker_npc::add_opinion( const npc_opinion &op )
{
    me_npc->op_of_u += op;
}

bool talker_npc::enslave_mind()
{
    bool not_following = g->get_follower_list().count( me_npc->getID() ) == 0;
    me_npc->companion_mission_role_id.clear();
    talk_function::follow( *me_npc );
    return not_following;
}

void talker_npc::set_first_topic( const std::string &chat_topic )
{
    me_npc->chatbin.first_topic = chat_topic;
}

bool talker_npc::is_safe() const
{
    return me_npc->is_safe();
}

void talker_npc::set_npc_trust( const int trust )
{
    me_npc->op_of_u.trust = trust;
}

int talker_npc::get_npc_trust() const
{
    return me_npc->op_of_u.trust;
}

void talker_npc::set_npc_fear( const int fear )
{
    me_npc->op_of_u.fear = fear;
}

int talker_npc::get_npc_fear() const
{
    return me_npc->op_of_u.fear;
}

void talker_npc::set_npc_value( const int value )
{
    me_npc->op_of_u.value = value;
}

int talker_npc::get_npc_value() const
{
    return me_npc->op_of_u.value;
}

void talker_npc::set_npc_anger( const int anger )
{
    me_npc->op_of_u.anger = anger;
}

int talker_npc::get_npc_anger() const
{
    return me_npc->op_of_u.anger;
}
