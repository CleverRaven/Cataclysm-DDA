#include "dialogue.h" // IWYU pragma: associated

#include <climits>
#include <cmath>
#include <cstddef>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <unordered_map>

#include "activity_type.h"
#include "avatar.h"
#include "cata_utility.h"
// needed for the workaround for the std::to_string bug in some compilers
#include "clzones.h"
#include "compatibility.h" // IWYU pragma: keep
#include "condition.h"
#include "debug.h"
#include "faction_camp.h"
#include "game.h"
#include "help.h"
#include "input.h"
#include "item.h"
#include "item_category.h"
#include "itype.h"
#include "json.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "mapgen_functions.h"
#include "martialarts.h"
#include "messages.h"
#include "mission.h"
#include "mission_companion.h"
#include "npc.h"
#include "npc_class.h"
#include "npctalk.h"
#include "npctrade.h"
#include "output.h"
#include "overmapbuffer.h"
#include "recipe.h"
#include "rng.h"
#include "skill.h"
#include "sounds.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "text_snippets.h"
#include "translations.h"
#include "ui.h"
#include "units.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "auto_pickup.h"
#include "bodypart.h"
#include "calendar.h"
#include "character.h"
#include "color.h"
#include "enums.h"
#include "faction.h"
#include "game_constants.h"
#include "int_id.h"
#include "mapdata.h"
#include "material.h"
#include "optional.h"
#include "pimpl.h"
#include "player_activity.h"
#include "player.h"
#include "point.h"

class basecamp;

const skill_id skill_speech( "speech" );
const skill_id skill_barter( "barter" );

const efftype_id effect_allow_sleep( "allow_sleep" );
const efftype_id effect_asked_for_item( "asked_for_item" );
const efftype_id effect_asked_personal_info( "asked_personal_info" );
const efftype_id effect_asked_to_follow( "asked_to_follow" );
const efftype_id effect_asked_to_lead( "asked_to_lead" );
const efftype_id effect_asked_to_train( "asked_to_train" );
const efftype_id effect_bite( "bite" );
const efftype_id effect_bleed( "bleed" );
const efftype_id effect_currently_busy( "currently_busy" );
const efftype_id effect_gave_quest_item( "gave_quest_item" );
const efftype_id effect_infected( "infected" );
const efftype_id effect_infection( "infection" );
const efftype_id effect_lying_down( "lying_down" );
const efftype_id effect_narcosis( "narcosis" );
const efftype_id effect_sleep( "sleep" );
const efftype_id effect_under_op( "under_operation" );
const efftype_id effect_riding( "riding" );

static const trait_id trait_DEBUG_MIND_CONTROL( "DEBUG_MIND_CONTROL" );
static const trait_id trait_PROF_FOODP( "PROF_FOODP" );

static const itype_id fuel_type_animal( "animal" );

const zone_type_id zone_no_investigate( "NPC_NO_INVESTIGATE" );
const zone_type_id zone_investigate_only( "NPC_INVESTIGATE_ONLY" );

static std::map<std::string, json_talk_topic> json_talk_topics;

// Every OWED_VAL that the NPC owes you counts as +1 towards convincing
#define OWED_VAL 1000

#define dbg(x) DebugLog((x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

int topic_category( const talk_topic &the_topic );

const talk_topic &special_talk( char ch );

std::string give_item_to( npc &p, bool allow_use, bool allow_carry );

std::string talk_trial::name() const
{
    static const std::array<std::string, NUM_TALK_TRIALS> texts = { {
            "", translate_marker( "LIE" ), translate_marker( "PERSUADE" ), translate_marker( "INTIMIDATE" ), ""
        }
    };
    if( static_cast<size_t>( type ) >= texts.size() ) {
        debugmsg( "invalid trial type %d", static_cast<int>( type ) );
        return std::string();
    }
    return texts[type].empty() ? std::string() : _( texts[type] );
}

/** Time (in turns) and cost (in cent) for training: */
time_duration calc_skill_training_time( const npc &p, const skill_id &skill )
{
    return 1_minutes + 30_seconds * g->u.get_skill_level( skill ) -
           1_seconds * p.get_skill_level( skill );
}

int calc_skill_training_cost( const npc &p, const skill_id &skill )
{
    if( p.is_player_ally() ) {
        return 0;
    }

    return 1000 * ( 1 + g->u.get_skill_level( skill ) ) * ( 1 + g->u.get_skill_level( skill ) );
}

// TODO: all styles cost the same and take the same time to train,
// maybe add values to the ma_style class to makes this variable
// TODO: maybe move this function into the ma_style class? Or into the NPC class?
time_duration calc_ma_style_training_time( const npc &, const matype_id & /* id */ )
{
    return 30_minutes;
}

int calc_ma_style_training_cost( const npc &p, const matype_id & /* id */ )
{
    if( p.is_player_ally() ) {
        return 0;
    }

    return 800;
}

int npc::calc_spell_training_cost( const bool knows, int difficulty, int level )
{
    if( is_player_ally() ) {
        return 0;
    }
    int ret = ( 100 * std::max( 1, difficulty ) * std::max( 1, level ) );
    if( !knows ) {
        ret = ret * 2;
    }
    return ret;
}

// Rescale values from "mission scale" to "opinion scale"
int npc_trading::cash_to_favor( const npc &, int cash )
{
    // TODO: It should affect different NPCs to a different degree
    // Square root of mission value in dollars
    // ~31 for zed mom, 50 for horde master, ~63 for plutonium cells
    double scaled_mission_val = sqrt( cash / 100.0 );
    return roll_remainder( scaled_mission_val );
}

enum npc_chat_menu {
    NPC_CHAT_DONE,
    NPC_CHAT_TALK,
    NPC_CHAT_YELL,
    NPC_CHAT_SENTENCE,
    NPC_CHAT_GUARD,
    NPC_CHAT_FOLLOW,
    NPC_CHAT_AWAKE,
    NPC_CHAT_MOUNT,
    NPC_CHAT_DISMOUNT,
    NPC_CHAT_DANGER,
    NPC_CHAT_ORDERS,
    NPC_CHAT_NO_GUNS,
    NPC_CHAT_PULP,
    NPC_CHAT_FOLLOW_CLOSE,
    NPC_CHAT_MOVE_FREELY,
    NPC_CHAT_SLEEP,
    NPC_CHAT_FORBID_ENGAGE,
    NPC_CHAT_CLEAR_OVERRIDES,
    NPC_CHAT_ANIMAL_VEHICLE_FOLLOW,
    NPC_CHAT_ANIMAL_VEHICLE_STOP_FOLLOW
};

// given a vector of NPCs, presents a menu to allow a player to pick one.
// everyone == true adds another entry at the end to allow selecting all listed NPCs
// this implies a return value of npc_list.size() means "everyone"
static int npc_select_menu( const std::vector<npc *> &npc_list, const std::string &prompt,
                            const bool everyone = true )
{
    if( npc_list.empty() ) {
        return -1;
    }
    const int npc_count = npc_list.size();
    if( npc_count == 1 ) {
        return 0;
    } else {
        uilist nmenu;
        nmenu.text = prompt;
        for( auto &elem : npc_list ) {
            nmenu.addentry( -1, true, MENU_AUTOASSIGN, ( elem )->name );
        }
        if( npc_count > 1 && everyone ) {
            nmenu.addentry( -1, true, MENU_AUTOASSIGN, _( "Everyone" ) );
        }
        nmenu.query();
        return nmenu.ret;
    }

}

static void npc_batch_override_toggle(
    const std::vector<npc *> &npc_list, ally_rule rule, bool state )
{
    for( npc *p : npc_list ) {
        p->rules.toggle_specific_override_state( rule, state );
    }
}

static void npc_temp_orders_menu( const std::vector<npc *> &npc_list )
{
    if( npc_list.empty() ) {
        return;
    }
    npc *guy = npc_list.front();

    bool done = false;
    uilist nmenu;

    while( !done ) {
        int override_count = 0;
        std::ostringstream override_string;
        override_string << string_format( _( "%s currently has these temporary orders:" ), guy->name );
        for( const auto &rule : ally_rule_strs ) {
            if( guy->rules.has_override_enable( rule.second.rule ) ) {
                override_count++;
                override_string << "\n  ";
                override_string << ( guy->rules.has_override( rule.second.rule ) ?
                                     rule.second.rule_true_text : rule.second.rule_false_text );
            }
        }
        if( override_count == 0 ) {
            override_string << "\n  " << _( "None." ) << "\n";
        }
        if( npc_list.size() > 1 ) {
            override_string << "\n" << _( "Other followers might have different temporary orders." );
        }
        g->refresh_all();
        nmenu.reset();
        nmenu.text = _( "Issue what temporary order?" );
        nmenu.desc_enabled = true;
        std::string output_string = override_string.str();
        parse_tags( output_string, g->u, *guy );
        nmenu.footer_text = output_string;
        nmenu.addentry( NPC_CHAT_DONE, true, 'd', _( "Done issuing orders" ) );
        nmenu.addentry( NPC_CHAT_FORBID_ENGAGE, true, 'f',
                        guy->rules.has_override_enable( ally_rule::forbid_engage ) ?
                        _( "Go back to your usual engagement habits" ) : _( "Don't engage hostiles for the time being" ) );
        nmenu.addentry( NPC_CHAT_NO_GUNS, true, 'g', guy->rules.has_override_enable( ally_rule::use_guns ) ?
                        _( "Use whatever weapon you normally would" ) : _( "Don't use ranged weapons for a while" ) );
        nmenu.addentry( NPC_CHAT_PULP, true, 'p', guy->rules.has_override_enable( ally_rule::allow_pulp ) ?
                        _( "Pulp zombies if you like" ) : _( "Hold off on pulping zombies for a while" ) );
        nmenu.addentry( NPC_CHAT_FOLLOW_CLOSE, true, 'c',
                        guy->rules.has_override_enable( ally_rule::follow_close ) &&
                        guy->rules.has_override( ally_rule::follow_close ) ?
                        _( "Go back to keeping your usual distance" ) : _( "Stick close to me for now" ) );
        nmenu.addentry( NPC_CHAT_MOVE_FREELY, true, 'm',
                        guy->rules.has_override_enable( ally_rule::follow_close ) &&
                        !guy->rules.has_override( ally_rule::follow_close ) ?
                        _( "Go back to keeping your usual distance" ) : _( "Move farther from me if you need to" ) );
        nmenu.addentry( NPC_CHAT_SLEEP, true, 's',
                        guy->rules.has_override_enable( ally_rule::allow_sleep ) ?
                        _( "Go back to your usual sleeping habits" ) : _( "Take a nap if you need it" ) );
        nmenu.addentry( NPC_CHAT_CLEAR_OVERRIDES, true, 'o', _( "Let's go back to your usual behaviors" ) );
        nmenu.query();

        switch( nmenu.ret ) {
            case NPC_CHAT_FORBID_ENGAGE:
                npc_batch_override_toggle( npc_list, ally_rule::forbid_engage, true );
                break;
            case NPC_CHAT_NO_GUNS:
                npc_batch_override_toggle( npc_list, ally_rule::use_guns, false );
                break;
            case NPC_CHAT_PULP:
                npc_batch_override_toggle( npc_list, ally_rule::allow_pulp, false );
                break;
            case NPC_CHAT_FOLLOW_CLOSE:
                npc_batch_override_toggle( npc_list, ally_rule::follow_close, true );
                break;
            case NPC_CHAT_MOVE_FREELY:
                npc_batch_override_toggle( npc_list, ally_rule::follow_close, false );
                break;
            case NPC_CHAT_SLEEP:
                npc_batch_override_toggle( npc_list, ally_rule::allow_sleep, true );
                break;
            case NPC_CHAT_CLEAR_OVERRIDES:
                for( npc *p : npc_list ) {
                    p->rules.clear_overrides();
                }
                break;
            default:
                done = true;
                break;
        }
    }

}

static void tell_veh_stop_following()
{
    for( auto &veh : g->m.get_vehicles() ) {
        auto &v = veh.v;
        if( v->has_engine_type( fuel_type_animal, false ) && v->is_owned_by( g->u ) ) {
            v->is_following = false;
            v->engine_on = false;
        }
    }
}

static void assign_veh_to_follow()
{
    for( auto &veh : g->m.get_vehicles() ) {
        auto &v = veh.v;
        if( v->has_engine_type( fuel_type_animal, false ) && v->is_owned_by( g->u ) ) {
            v->activate_animal_follow();
        }
    }
}

void game::chat()
{
    int volume = g->u.get_shout_volume();

    const std::vector<npc *> available = get_npcs_if( [&]( const npc & guy ) {
        // TODO: Get rid of the z-level check when z-level vision gets "better"
        return u.posz() == guy.posz() && u.sees( guy.pos() ) &&
               rl_dist( u.pos(), guy.pos() ) <= SEEX * 2;
    } );
    const int available_count = available.size();
    const std::vector<npc *> followers = get_npcs_if( [&]( const npc & guy ) {
        return guy.is_player_ally() && guy.is_following() && guy.can_hear( u.pos(), volume );
    } );
    const int follower_count = followers.size();
    const std::vector<npc *> guards = get_npcs_if( [&]( const npc & guy ) {
        return guy.mission == NPC_MISSION_GUARD_ALLY &&
               guy.companion_mission_role_id != "FACTION_CAMP" &&
               guy.can_hear( u.pos(), volume );
    } );
    const int guard_count = guards.size();

    if( g->u.has_trait( trait_PROF_FOODP ) && !( g->u.is_wearing( itype_id( "foodperson_mask" ) ) ||
            g->u.is_wearing( itype_id( "foodperson_mask_on" ) ) ) ) {
        g->u.add_msg_if_player( m_warning, _( "You can't speak without your face!" ) );
        return;
    }
    std::vector<vehicle *> animal_vehicles;
    std::vector<vehicle *> following_vehicles;
    for( auto &veh : g->m.get_vehicles() ) {
        auto &v = veh.v;
        if( v->has_engine_type( fuel_type_animal, false ) && v->is_owned_by( g->u ) ) {
            animal_vehicles.push_back( v );
            if( v->is_following ) {
                following_vehicles.push_back( v );
            }
        }
    }

    uilist nmenu;
    nmenu.text = std::string( _( "What do you want to do?" ) );

    if( !available.empty() ) {
        nmenu.addentry( NPC_CHAT_TALK, true, 't', available_count == 1 ?
                        string_format( _( "Talk to %s" ), available.front()->name ) :
                        _( "Talk to ..." )
                      );
    }
    nmenu.addentry( NPC_CHAT_YELL, true, 'a', _( "Yell" ) );
    nmenu.addentry( NPC_CHAT_SENTENCE, true, 'b', _( "Yell a sentence" ) );
    if( !animal_vehicles.empty() ) {
        nmenu.addentry( NPC_CHAT_ANIMAL_VEHICLE_FOLLOW, true, 'F',
                        _( "Whistle at your animals pulling vehicles to follow you." ) );
    }
    if( !following_vehicles.empty() ) {
        nmenu.addentry( NPC_CHAT_ANIMAL_VEHICLE_STOP_FOLLOW, true, 'S',
                        _( "Whistle at your animals pulling vehicles to stop following you." ) );
    }
    if( !guards.empty() ) {
        nmenu.addentry( NPC_CHAT_FOLLOW, true, 'f', guard_count == 1 ?
                        string_format( _( "Tell %s to follow" ), guards.front()->name ) :
                        _( "Tell someone to follow..." )
                      );
    }
    if( !followers.empty() ) {
        nmenu.addentry( NPC_CHAT_GUARD, true, 'g', follower_count == 1 ?
                        string_format( _( "Tell %s to guard" ), followers.front()->name ) :
                        _( "Tell someone to guard..." )
                      );
        nmenu.addentry( NPC_CHAT_AWAKE, true, 'w', _( "Tell everyone on your team to wake up" ) );
        nmenu.addentry( NPC_CHAT_MOUNT, true, 'M', _( "Tell everyone on your team to mount up" ) );
        nmenu.addentry( NPC_CHAT_DISMOUNT, true, 'm', _( "Tell everyone on your team to dismount" ) );
        nmenu.addentry( NPC_CHAT_DANGER, true, 'D',
                        _( "Tell everyone on your team to prepare for danger" ) );
        nmenu.addentry( NPC_CHAT_CLEAR_OVERRIDES, true, 'r',
                        _( "Tell everyone on your team to relax (Clear Overrides)" ) );
        nmenu.addentry( NPC_CHAT_ORDERS, true, 'o', _( "Tell everyone on your team to temporarily..." ) );
    }
    std::string message;
    std::string yell_msg;
    bool is_order = true;
    nmenu.query();

    if( nmenu.ret < 0 ) {
        return;
    }

    switch( nmenu.ret ) {
        case NPC_CHAT_TALK: {
            const int npcselect = npc_select_menu( available, _( "Talk to whom?" ), false );
            if( npcselect < 0 ) {
                return;
            }
            available[npcselect]->talk_to_u();
            break;
        }
        case NPC_CHAT_YELL:
            is_order = false;
            message = _( "loudly." );
            break;
        case NPC_CHAT_SENTENCE: {
            std::string popupdesc = _( "Enter a sentence to yell" );
            string_input_popup popup;
            popup.title( _( "Yell a sentence" ) )
            .width( 64 )
            .description( popupdesc )
            .identifier( "sentence" )
            .max_length( 128 )
            .query();
            yell_msg = popup.text() + ".";
            is_order = false;
            break;
        }
        case NPC_CHAT_GUARD: {
            const int npcselect = npc_select_menu( followers, _( "Who should guard here?" ) );
            if( npcselect < 0 ) {
                return;
            }
            if( npcselect == follower_count ) {
                for( npc *them : followers ) {
                    talk_function::assign_guard( *them );
                }
                yell_msg = _( "Everyone guard here!" );
            } else {
                talk_function::assign_guard( *followers[npcselect] );
                yell_msg = string_format( _( "Guard here, %s!" ), followers[npcselect]->name );
            }
            break;
        }
        case NPC_CHAT_FOLLOW: {
            const int npcselect = npc_select_menu( guards, _( "Who should follow you?" ) );
            if( npcselect < 0 ) {
                return;
            }
            if( npcselect == guard_count ) {
                for( npc *them : guards ) {
                    talk_function::stop_guard( *them );
                }
                yell_msg = _( "Everyone follow me!" );
            } else {
                talk_function::stop_guard( *guards[npcselect] );
                yell_msg = string_format( _( "Follow me, %s!" ), guards[npcselect]->name );
            }
            break;
        }
        case NPC_CHAT_AWAKE:
            for( npc *them : followers ) {
                talk_function::wake_up( *them );
            }
            yell_msg = _( "Stay awake!" );
            break;
        case NPC_CHAT_MOUNT:
            for( npc *them : followers ) {
                if( them->has_effect( effect_riding ) ) {
                    continue;
                }
                talk_function::find_mount( *them );
            }
            yell_msg = _( "Mount up!" );
            break;
        case NPC_CHAT_DISMOUNT:
            for( npc *them : followers ) {
                if( them->has_effect( effect_riding ) ) {
                    them->npc_dismount();
                }
            }
            yell_msg = _( "Dismount!" );
            break;
        case NPC_CHAT_DANGER:
            for( npc *them : followers ) {
                them->rules.set_danger_overrides();
            }
            yell_msg = _( "We're in danger.  Stay awake, stay close, don't go wandering off, "
                          "and don't open any doors." );
            break;
        case NPC_CHAT_CLEAR_OVERRIDES:
            for( npc *p : followers ) {
                talk_function::clear_overrides( *p );
            }
            yell_msg = _( "As you were." );
            break;
        case NPC_CHAT_ORDERS:
            npc_temp_orders_menu( followers );
            break;
        case NPC_CHAT_ANIMAL_VEHICLE_FOLLOW:
            assign_veh_to_follow();
            break;
        case NPC_CHAT_ANIMAL_VEHICLE_STOP_FOLLOW:
            tell_veh_stop_following();
            break;
        default:
            return;
    }

    if( !yell_msg.empty() ) {
        message = string_format( "\"%s\"", yell_msg );
    }
    if( !message.empty() ) {
        add_msg( _( "You yell %s" ), message );
        u.shout( string_format( _( "%s yelling %s" ), u.disp_name(), message ), is_order );
    }

    u.moves -= 100;
    refresh_all();
}

void npc::handle_sound( int priority, const std::string &description, int heard_volume,
                        const tripoint &spos )
{
    const tripoint s_abs_pos = g->m.getabs( spos );
    const tripoint my_abs_pos = g->m.getabs( pos() );

    add_msg( m_debug, "%s heard '%s', priority %d at volume %d from %d:%d, my pos %d:%d",
             disp_name(), description, priority, heard_volume, s_abs_pos.x, s_abs_pos.y,
             my_abs_pos.x, my_abs_pos.y );

    const sounds::sound_t spriority = static_cast<sounds::sound_t>( priority );
    bool player_ally = g->u.pos() == spos && is_player_ally();
    player *const sound_source = g->critter_at<player>( spos );
    bool npc_ally = sound_source && sound_source->is_npc() && is_ally( *sound_source );

    if( ( player_ally || npc_ally ) && spriority == sounds::sound_t::order ) {
        say( "<acknowledged>" );
    }

    if( sees( spos ) || is_hallucination() ) {
        return;
    }
    // ignore low priority sounds if the NPC "knows" it came from a friend.
    // @ todo NPC will need to respond to talking noise eventually
    // but only for bantering purposes, not for investigating.
    if( spriority < sounds::sound_t::alarm ) {
        if( player_ally ) {
            add_msg( m_debug, "Allied NPC ignored same faction %s", name );
            return;
        }
        if( npc_ally ) {
            add_msg( m_debug, "NPC ignored same faction %s", name );
            return;
        }
    }
    // discount if sound source is player, or seen by player,
    // and listener is friendly and sound source is combat or alert only.
    if( spriority < sounds::sound_t::alarm && g->u.sees( spos ) ) {
        if( is_player_ally() ) {
            add_msg( m_debug, "NPC %s ignored low priority noise that player can see", name );
            return;
            // discount if sound source is player, or seen by player,
            // listener is neutral and sound type is worth investigating.
        } else if( spriority < sounds::sound_t::destructive_activity &&
                   get_attitude_group( get_attitude() ) != attitude_group::hostile ) {
            return;
        }
    }
    // patrolling guards will investigate more readily than stationary NPCS
    int investigate_dist = 10;
    if( mission == NPC_MISSION_GUARD_ALLY || mission == NPC_MISSION_GUARD_PATROL ) {
        investigate_dist = 50;
    }
    if( rules.has_flag( ally_rule::ignore_noise ) ) {
        investigate_dist = 0;
    }
    if( ai_cache.total_danger < 1.0f ) {
        if( spriority == sounds::sound_t::movement && !in_vehicle ) {
            warn_about( "movement_noise", rng( 1, 10 ) * 1_minutes, description );
        } else if( spriority > sounds::sound_t::movement ) {
            if( ( spriority == sounds::sound_t::speech || spriority == sounds::sound_t::alert ||
                  spriority == sounds::sound_t::order ) && sound_source &&
                !has_faction_relationship( *sound_source, npc_factions::knows_your_voice ) ) {
                warn_about( "speech_noise", rng( 1, 10 ) * 1_minutes );
            } else if( spriority > sounds::sound_t::activity ) {
                warn_about( "combat_noise", rng( 1, 10 ) * 1_minutes );
            }
            bool should_check = rl_dist( pos(), spos ) < investigate_dist;
            if( should_check ) {
                const zone_manager &mgr = zone_manager::get_manager();
                if( mgr.has( zone_no_investigate, s_abs_pos, fac_id ) ) {
                    should_check = false;
                } else if( mgr.has( zone_investigate_only, my_abs_pos, fac_id ) &&
                           !mgr.has( zone_investigate_only, s_abs_pos, fac_id ) ) {
                    should_check = false;
                }
            }
            if( should_check ) {
                add_msg( m_debug, "%s added noise at pos %d:%d", name, s_abs_pos.x, s_abs_pos.y );
                dangerous_sound temp_sound;
                temp_sound.abs_pos = s_abs_pos;
                temp_sound.volume = heard_volume;
                temp_sound.type = priority;
                if( !ai_cache.sound_alerts.empty() ) {
                    if( ai_cache.sound_alerts.back().abs_pos != s_abs_pos ) {
                        ai_cache.sound_alerts.push_back( temp_sound );
                    }
                } else {
                    ai_cache.sound_alerts.push_back( temp_sound );
                }
            }
        }
    }
}

void npc_chatbin::check_missions()
{
    // TODO: or simply fail them? Some missions might only need to be reported.
    auto &ma = missions_assigned;
    const auto last = std::remove_if( ma.begin(), ma.end(), []( class mission const * m ) {
        return !m->is_assigned();
    } );
    std::copy( last, ma.end(), std::back_inserter( missions ) );
    ma.erase( last, ma.end() );
}

void npc::talk_to_u( bool text_only, bool radio_contact )
{
    if( g->u.is_dead_state() ) {
        set_attitude( NPCATT_NULL );
        return;
    }
    const bool has_mind_control = g->u.has_trait( trait_DEBUG_MIND_CONTROL );
    // This is necessary so that we don't bug the player over and over
    if( get_attitude() == NPCATT_TALK ) {
        set_attitude( NPCATT_NULL );
    } else if( !has_mind_control && ( get_attitude() == NPCATT_FLEE ||
                                      get_attitude() == NPCATT_FLEE_TEMP ) ) {
        add_msg( _( "%s is fleeing from you!" ), name );
        return;
    } else if( !has_mind_control && get_attitude() == NPCATT_KILL ) {
        add_msg( _( "%s is hostile!" ), name );
        return;
    }
    if( get_faction() ) {
        get_faction()->known_by_u = true;
    }
    set_known_to_u( true );
    dialogue d;
    d.alpha = &g->u;
    d.beta = this;

    chatbin.check_missions();

    // For each active mission we have, let the mission know we talked to this NPC.
    for( auto &mission : g->u.get_active_missions() ) {
        mission->on_talk_with_npc( this->getID() );
    }

    for( auto &mission : chatbin.missions_assigned ) {
        if( mission->get_assigned_player_id() == g->u.getID() ) {
            d.missions_assigned.push_back( mission );
        }
    }
    d.add_topic( chatbin.first_topic );
    if( radio_contact ) {
        d.add_topic( "TALK_RADIO" );
        d.by_radio = true;
    } else if( is_leader() ) {
        d.add_topic( "TALK_LEADER" );
    } else if( is_player_ally() && ( is_walking_with() || has_activity() ) ) {
        d.add_topic( "TALK_FRIEND" );
    } else if( get_attitude() == NPCATT_RECOVER_GOODS ) {
        d.add_topic( "TALK_STOLE_ITEM" );
    }
    int most_difficult_mission = 0;
    for( auto &mission : chatbin.missions ) {
        const auto &type = mission->get_type();
        if( type.urgent && type.difficulty > most_difficult_mission ) {
            d.add_topic( "TALK_MISSION_DESCRIBE_URGENT" );
            chatbin.mission_selected = mission;
            most_difficult_mission = type.difficulty;
        }
    }
    most_difficult_mission = 0;
    bool chosen_urgent = false;
    for( auto &mission : chatbin.missions_assigned ) {
        if( mission->get_assigned_player_id() != g->u.getID() ) {
            // Not assigned to the player that is currently talking to the npc
            continue;
        }
        const auto &type = mission->get_type();
        if( ( type.urgent && !chosen_urgent ) || ( type.difficulty > most_difficult_mission &&
                ( type.urgent || !chosen_urgent ) ) ) {
            chosen_urgent = type.urgent;
            d.add_topic( "TALK_MISSION_INQUIRE" );
            chatbin.mission_selected = mission;
            most_difficult_mission = type.difficulty;
        }
    }
    if( chatbin.mission_selected != nullptr ) {
        if( chatbin.mission_selected->get_assigned_player_id() != g->u.getID() ) {
            // Don't talk about a mission that is assigned to someone else.
            chatbin.mission_selected = nullptr;
        }
    }
    if( chatbin.mission_selected == nullptr ) {
        // if possible, select a mission to talk about
        if( !chatbin.missions.empty() ) {
            chatbin.mission_selected = chatbin.missions.front();
        } else if( !d.missions_assigned.empty() ) {
            chatbin.mission_selected = d.missions_assigned.front();
        }
    }

    // Needs
    if( has_effect( effect_sleep ) || has_effect( effect_lying_down ) ) {
        if( has_effect( effect_narcosis ) ) {
            d.add_topic( "TALK_SEDATED" );
        } else {
            d.add_topic( "TALK_WAKE_UP" );
        }
    }

    if( d.topic_stack.back().id == "TALK_NONE" ) {
        d.topic_stack.back() = talk_topic( pick_talk_topic( g->u ) );
    }

    moves -= 100;

    if( g->u.is_deaf() ) {
        if( d.topic_stack.back().id == "TALK_MUG" ||
            d.topic_stack.back().id == "TALK_STRANGER_AGGRESSIVE" ) {
            make_angry();
            d.add_topic( "TALK_DEAF_ANGRY" );
        } else {
            d.add_topic( "TALK_DEAF" );
        }
    }

    if( g->u.has_trait( trait_PROF_FOODP ) && !( g->u.is_wearing( itype_id( "foodperson_mask" ) ) ||
            g->u.is_wearing( itype_id( "foodperson_mask_on" ) ) ) ) {
        d.add_topic( "TALK_NOFACE" );
    }

    if( has_trait( trait_PROF_FOODP ) && !( is_wearing( itype_id( "foodperson_mask" ) ) ||
                                            is_wearing( itype_id( "foodperson_mask_on" ) ) ) ) {
        d.add_topic( "TALK_NPC_NOFACE" );
    }

    decide_needs();

    dialogue_window d_win;
    d_win.open_dialogue( text_only );
    // Main dialogue loop
    do {
        d_win.print_header( name );
        const talk_topic next = d.opt( d_win, d.topic_stack.back() );
        if( next.id == "TALK_NONE" ) {
            int cat = topic_category( d.topic_stack.back() );
            do {
                d.topic_stack.pop_back();
            } while( cat != -1 && topic_category( d.topic_stack.back() ) == cat );
        }
        if( next.id == "TALK_DONE" || d.topic_stack.empty() ) {
            d.beta->say( _( "Bye." ) );
            d.done = true;
        } else if( next.id != "TALK_NONE" ) {
            d.add_topic( next );
        }
    } while( !d.done );
    g->refresh_all();

    if( g->u.activity.id() == activity_id( "ACT_AIM" ) && !g->u.has_weapon() ) {
        g->u.cancel_activity();
        // don't query certain activities that are started from dialogue
    } else if( g->u.activity.id() == activity_id( "ACT_TRAIN" ) ||
               g->u.activity.id() == activity_id( "ACT_WAIT_NPC" ) ||
               g->u.activity.id() == activity_id( "ACT_SOCIALIZE" ) ||
               g->u.activity.index == getID().get_value() ) {
        return;
    }

    if( !g->u.has_effect( effect_under_op ) ) {
        g->cancel_activity_or_ignore_query( distraction_type::talked_to,
                                            string_format( _( "%s talked to you." ), name ) );
    }
}

std::string dialogue::dynamic_line( const talk_topic &the_topic ) const
{
    if( the_topic.item_type != "null" ) {
        cur_item = the_topic.item_type;
    }

    // For compatibility
    const auto &topic = the_topic.id;
    const auto iter = json_talk_topics.find( topic );
    if( iter != json_talk_topics.end() ) {
        const std::string line = iter->second.get_dynamic_line( *this );
        if( !line.empty() ) {
            return line;
        }
    }

    if( topic == "TALK_NPC_NOFACE" ) {
        return string_format( _( "&%s stays silent." ), beta->name );
    }

    if( topic == "TALK_NOFACE" ) {
        return _( "&You can't talk without your face." );
    } else if( topic == "TALK_DEAF" ) {
        return _( "&You are deaf and can't talk." );

    } else if( topic == "TALK_DEAF_ANGRY" ) {
        return string_format(
                   _( "&You are deaf and can't talk. When you don't respond, %s becomes angry!" ),
                   beta->name );
    }
    if( topic == "TALK_SEDATED" ) {
        return string_format(
                   _( "%1$s is sedated and can't be moved or woken up until the medication or sedation wears off.\nYou estimate it will wear off in %2$s." ),
                   beta->name, to_string_approx( g->u.estimate_effect_dur( skill_id( "firstaid" ), effect_narcosis,
                           15_minutes, 6, *beta ) ) );
    }

    const auto &p = beta; // for compatibility, later replace it in the code below
    // Those topics are handled by the mission system, see there.
    static const std::unordered_set<std::string> mission_topics = { {
            "TALK_MISSION_DESCRIBE", "TALK_MISSION_DESCRIBE_URGENT",
            "TALK_MISSION_OFFER", "TALK_MISSION_ACCEPTED",
            "TALK_MISSION_REJECTED", "TALK_MISSION_ADVICE", "TALK_MISSION_INQUIRE",
            "TALK_MISSION_SUCCESS", "TALK_MISSION_SUCCESS_LIE", "TALK_MISSION_FAILURE"
        }
    };
    if( mission_topics.count( topic ) > 0 ) {
        if( p->chatbin.mission_selected == nullptr ) {
            return "mission_selected == nullptr; BUG! (npctalk.cpp:dynamic_line)";
        }
        mission *miss = p->chatbin.mission_selected;
        const auto &type = miss->get_type();
        // TODO: make it a member of the mission class, maybe at mission instance specific data
        const std::string &ret = miss->dialogue_for_topic( topic );
        if( ret.empty() ) {
            debugmsg( "Bug in npctalk.cpp:dynamic_line. Wrong mission_id(%s) or topic(%s)",
                      type.id.c_str(), topic.c_str() );
            return "";
        }

        if( topic == "TALK_MISSION_SUCCESS" && miss->has_follow_up() ) {
            switch( rng( 1, 3 ) ) {
                case 1:
                    return ret + _( "  And I have more I'd like you to do." );
                case 2:
                    return ret + _( "  I could use a hand with something else if you are interested." );
                case 3:
                    return ret + _( "  If you are interested, I have another job for you." );
            }
        }

        return ret;
    }

    if( topic == "TALK_NONE" || topic == "TALK_DONE" ) {
        return _( "Bye." );
    } else if( topic == "TALK_TRAIN" ) {
        if( !g->u.backlog.empty() && g->u.backlog.front().id() == activity_id( "ACT_TRAIN" ) ) {
            return _( "Shall we resume?" );
        }
        std::vector<skill_id> trainable = p->skills_offered_to( g->u );
        std::vector<matype_id> styles = p->styles_offered_to( g->u );
        const std::vector<spell_id> spells = p->magic.spells();
        std::vector<spell_id> teachable_spells;
        for( const spell_id &sp : spells ) {
            if( g->u.magic.can_learn_spell( g->u, sp ) ) {
                teachable_spells.emplace_back( sp );
            }
        }
        if( trainable.empty() && styles.empty() && teachable_spells.empty() ) {
            return _( "Sorry, but it doesn't seem I have anything to teach you." );
        } else {
            return _( "Here's what I can teach you..." );
        }
    } else if( topic == "TALK_HOW_MUCH_FURTHER" ) {
        // TODO: this ignores the z-component
        const tripoint player_pos = p->global_omt_location();
        int dist = rl_dist( player_pos, p->goal );
        std::string response;
        dist *= 100;
        if( dist >= 1300 ) {
            int miles = dist / 25; // *100, e.g. quarter mile is "25"
            miles -= miles % 25; // Round to nearest quarter-mile
            int fullmiles = ( miles - miles % 100 ) / 100; // Left of the decimal point
            if( fullmiles <= 0 ) {
                fullmiles = 0;
            }
            response = string_format( _( "%d.%d miles." ), fullmiles, miles );
        } else {
            response = string_format( ngettext( "%d foot.", "%d feet.", dist ), dist );
        }
        return response;
    } else if( topic == "TALK_DESCRIBE_MISSION" ) {
        switch( p->mission ) {
            case NPC_MISSION_SHELTER:
                return string_format( _( "I'm holing up here for safety.  Long term, %s" ),
                                      p->myclass.obj().get_job_description() );
            case NPC_MISSION_SHOPKEEP:
                return _( "I run the shop here." );
            case NPC_MISSION_GUARD:
            case NPC_MISSION_GUARD_ALLY:
            case NPC_MISSION_GUARD_PATROL:
                return string_format( _( "Currently, I'm guarding this location.  Overall, %s" ),
                                      p->myclass.obj().get_job_description() );
            case NPC_MISSION_ACTIVITY:
                return string_format( _( "Right now, I'm <current_activity>.  In general, %s" ),
                                      p->myclass.obj().get_job_description() );
            case NPC_MISSION_TRAVELLING:
            case NPC_MISSION_NULL:
                return p->myclass.obj().get_job_description();
            default:
                return string_format( "ERROR: Someone forgot to code an npc_mission text for "
                                      "mission: %d.", static_cast<int>( p->mission ) );
        } // switch (p->mission)
    } else if( topic == "TALK_SHOUT" ) {
        alpha->shout();
        if( alpha->is_deaf() ) {
            return _( "&You yell, but can't hear yourself." );
        } else {
            return _( "&You yell." );
        }
    } else if( topic == "TALK_SIZE_UP" ) {
        ///\EFFECT_PER affects whether player can size up NPCs

        ///\EFFECT_INT slightly affects whether player can size up NPCs
        int ability = g->u.per_cur * 3 + g->u.int_cur;
        if( ability <= 10 ) {
            return _( "&You can't make anything out." );
        }

        if( p->is_player_ally() || ability > 100 ) {
            ability = 100;
        }

        std::string info = "&";
        int str_range = static_cast<int>( 100 / ability );
        int str_min = static_cast<int>( p->str_max / str_range ) * str_range;
        info += string_format( _( "Str %d - %d" ), str_min, str_min + str_range );

        if( ability >= 40 ) {
            int dex_range = static_cast<int>( 160 / ability );
            int dex_min = static_cast<int>( p->dex_max / dex_range ) * dex_range;
            info += string_format( _( "  Dex %d - %d" ), dex_min, dex_min + dex_range );
        }

        if( ability >= 50 ) {
            int int_range = static_cast<int>( 200 / ability );
            int int_min = static_cast<int>( p->int_max / int_range ) * int_range;
            info += string_format( _( "  Int %d - %d" ), int_min, int_min + int_range );
        }

        if( ability >= 60 ) {
            int per_range = static_cast<int>( 240 / ability );
            int per_min = static_cast<int>( p->per_max / per_range ) * per_range;
            info += string_format( _( "  Per %d - %d" ), per_min, per_min + per_range );
        }

        needs_rates rates = p->calc_needs_rates();
        if( ability >= 100 - ( p->get_fatigue() / 10 ) ) {
            std::string how_tired;
            if( p->get_fatigue() > EXHAUSTED ) {
                how_tired = _( "Exhausted" );
            } else if( p->get_fatigue() > DEAD_TIRED ) {
                how_tired = _( "Dead tired" );
            } else if( p->get_fatigue() > TIRED ) {
                how_tired = _( "Tired" );
            } else {
                how_tired = _( "Not tired" );
                if( ability >= 100 ) {
                    time_duration sleep_at = 5_minutes * ( TIRED - p->get_fatigue() ) /
                                             rates.fatigue;
                    how_tired += _( ".  Will need sleep in " ) + to_string_approx( sleep_at );
                }
            }
            info += "\n" + how_tired;
        }
        if( ability >= 100 ) {
            if( p->get_thirst() < 100 ) {
                time_duration thirst_at = 5_minutes * ( 100 - p->get_thirst() ) / rates.thirst;
                if( thirst_at > 1_hours ) {
                    info += _( "\nWill need water in " ) + to_string_approx( thirst_at );
                }
            } else {
                info += _( "\nThirsty" );
            }
            if( p->get_hunger() < 100 ) {
                time_duration hunger_at = 5_minutes * ( 100 - p->get_hunger() ) / rates.hunger;
                if( hunger_at > 1_hours ) {
                    info += _( "\nWill need food in " ) + to_string_approx( hunger_at );
                }
            } else {
                info += _( "\nHungry" );
            }
        }
        return info;
    } else if( topic == "TALK_LOOK_AT" ) {
        return "&" + p->short_description();
    } else if( topic == "TALK_OPINION" ) {
        return "&" + p->opinion_text();
    } else if( topic == "TALK_MIND_CONTROL" ) {
        bool not_following = g->get_follower_list().count( p->getID() ) == 0;
        p->companion_mission_role_id.clear();
        talk_function::follow( *p );
        if( not_following ) {
            return _( "YES, MASTER!" );
        }
    }

    return string_format( "I don't know what to say for %s. (BUG (npctalk.cpp:dynamic_line))",
                          topic );
}

void dialogue::apply_speaker_effects( const talk_topic &the_topic )
{
    const auto &topic = the_topic.id;
    const auto iter = json_talk_topics.find( topic );
    if( iter == json_talk_topics.end() ) {
        return;
    }
    for( json_dynamic_line_effect &npc_effect : iter->second.get_speaker_effects() ) {
        if( npc_effect.test_condition( *this ) ) {
            npc_effect.apply( *this );
        }
    }
}

talk_response &dialogue::add_response( const std::string &text, const std::string &r,
                                       const bool first )
{
    talk_response result = talk_response();
    result.truetext = no_translation( text );
    result.truefalse_condition = []( const dialogue & ) {
        return true;
    };
    result.success.next_topic = talk_topic( r );
    if( first ) {
        responses.insert( responses.begin(), result );
        return responses.front();
    } else {
        responses.push_back( result );
        return responses.back();
    }
}

talk_response &dialogue::add_response_done( const std::string &text )
{
    return add_response( text, "TALK_DONE" );
}

talk_response &dialogue::add_response_none( const std::string &text )
{
    return add_response( text, "TALK_NONE" );
}

talk_response &dialogue::add_response( const std::string &text, const std::string &r,
                                       talkfunction_ptr effect_success, const bool first )
{
    talk_response &result = add_response( text, r, first );
    result.success.set_effect( effect_success );
    return result;
}

talk_response &dialogue::add_response( const std::string &text, const std::string &r,
                                       std::function<void( npc & )> effect_success,
                                       dialogue_consequence consequence, const bool first )
{
    talk_response &result = add_response( text, r, first );
    result.success.set_effect_consequence( effect_success, consequence );
    return result;
}

talk_response &dialogue::add_response( const std::string &text, const std::string &r,
                                       mission *miss, const bool first )
{
    if( miss == nullptr ) {
        debugmsg( "tried to select null mission" );
    }
    talk_response &result = add_response( text, r, first );
    result.mission_selected = miss;
    return result;
}

talk_response &dialogue::add_response( const std::string &text, const std::string &r,
                                       const skill_id &skill, const bool first )
{
    talk_response &result = add_response( text, r, first );
    result.skill = skill;
    return result;
}

talk_response &dialogue::add_response( const std::string &text, const std::string &r,
                                       const spell_id &sp, const bool first )
{
    talk_response &result = add_response( text, r, first );
    result.dialogue_spell = sp;
    return result;
}

talk_response &dialogue::add_response( const std::string &text, const std::string &r,
                                       const martialart &style, const bool first )
{
    talk_response &result = add_response( text, r, first );
    result.style = style.id;
    return result;
}

talk_response &dialogue::add_response( const std::string &text, const std::string &r,
                                       const itype_id &item_type, const bool first )
{
    if( item_type == "null" ) {
        debugmsg( "explicitly specified null item" );
    }

    talk_response &result = add_response( text, r, first );
    result.success.next_topic.item_type = item_type;
    return result;
}

void dialogue::gen_responses( const talk_topic &the_topic )
{
    const auto &topic = the_topic.id; // for compatibility, later replace it in the code below
    const auto p = beta; // for compatibility, later replace it in the code below
    auto &ret = responses; // for compatibility, later replace it in the code below
    ret.clear();
    const auto iter = json_talk_topics.find( topic );
    if( iter != json_talk_topics.end() ) {
        json_talk_topic &jtt = iter->second;
        if( jtt.gen_responses( *this ) ) {
            return;
        }
    }

    if( topic == "TALK_MISSION_LIST" ) {
        if( p->chatbin.missions.size() == 1 ) {
            add_response( _( "Tell me about it." ), "TALK_MISSION_OFFER",
                          p->chatbin.missions.front(), true );
        } else {
            for( auto &mission : p->chatbin.missions ) {
                add_response( mission->get_type().tname(), "TALK_MISSION_OFFER", mission, true );
            }
        }
    } else if( topic == "TALK_MISSION_LIST_ASSIGNED" ) {
        if( missions_assigned.size() == 1 ) {
            add_response( _( "I have news." ), "TALK_MISSION_INQUIRE", missions_assigned.front() );
        } else {
            for( auto &miss_it : missions_assigned ) {
                add_response( miss_it->get_type().tname(), "TALK_MISSION_INQUIRE", miss_it );
            }
        }
    } else if( topic == "TALK_TRAIN" ) {
        if( !g->u.backlog.empty() && g->u.backlog.front().id() == activity_id( "ACT_TRAIN" ) &&
            g->u.backlog.front().index == p->getID().get_value() ) {
            player_activity &backlog = g->u.backlog.front();
            std::stringstream resume;
            resume << _( "Yes, let's resume training " );
            const skill_id skillt( backlog.name );
            // TODO: This is potentially dangerous. A skill and a martial art
            // could have the same ident!
            if( !skillt.is_valid() ) {
                const matype_id styleid = matype_id( backlog.name );
                if( !styleid.is_valid() ) {
                    const spell_id &sp_id = spell_id( backlog.name );
                    if( p->magic.knows_spell( sp_id ) ) {
                        resume << sp_id->name.translated();
                        add_response( resume.str(), "TALK_TRAIN_START", sp_id );
                    }
                } else {
                    martialart style = styleid.obj();
                    resume << style.name;
                    add_response( resume.str(), "TALK_TRAIN_START", style );
                }
            } else {
                resume << skillt.obj().name();
                add_response( resume.str(), "TALK_TRAIN_START", skillt );
            }
        }
        std::vector<matype_id> styles = p->styles_offered_to( g->u );
        std::vector<skill_id> trainable = p->skills_offered_to( g->u );
        const std::vector<spell_id> spells = p->magic.spells();
        std::vector<spell_id> teachable_spells;
        for( const spell_id &sp : spells ) {
            const spell &temp_spell = p->magic.get_spell( sp );
            if( g->u.magic.can_learn_spell( g->u, sp ) ) {
                if( g->u.magic.knows_spell( sp ) ) {
                    const spell &player_spell = g->u.magic.get_spell( sp );
                    if( player_spell.is_max_level() || player_spell.get_level() >= temp_spell.get_level() ) {
                        continue;
                    }
                }
                teachable_spells.emplace_back( sp );
            }
        }
        if( trainable.empty() && styles.empty() && teachable_spells.empty() ) {
            add_response_none( _( "Oh, okay." ) );
            return;
        }
        for( const spell_id &sp : teachable_spells ) {
            const spell &temp_spell = p->magic.get_spell( sp );
            const bool knows = g->u.magic.knows_spell( sp );
            const int cost = p->calc_spell_training_cost( knows, temp_spell.get_difficulty(),
                             temp_spell.get_level() );
            std::string text;
            if( knows ) {
                text = string_format( _( "%s: 1 hour lesson (cost %s)" ), temp_spell.name(),
                                      format_money( cost ) );
            } else {
                text = string_format( _( "%s: teaching spell knowledge (cost %s)" ), temp_spell.name(),
                                      format_money( cost ) );
            }
            add_response( text, "TALK_TRAIN_START", sp );
        }
        for( auto &style_id : styles ) {
            auto &style = style_id.obj();
            const int cost = calc_ma_style_training_cost( *p, style.id );
            //~Martial art style (cost in dollars)
            const std::string text = string_format( cost > 0 ? _( "%s ( cost $%d )" ) : "%s",
                                                    style.name, cost / 100 );
            add_response( text, "TALK_TRAIN_START", style );
        }
        for( auto &trained : trainable ) {
            const int cost = calc_skill_training_cost( *p, trained );
            SkillLevel skill_level_obj = g->u.get_skill_level_object( trained );
            const int cur_level = skill_level_obj.level();
            const int cur_level_exercise = skill_level_obj.exercise();
            skill_level_obj.train( 100 );
            const int next_level = skill_level_obj.level();
            const int next_level_exercise = skill_level_obj.exercise();

            //~Skill name: current level (exercise) -> next level (exercise) (cost in dollars)
            std::string text = string_format( cost > 0 ? _( "%s: %d (%d%%) -> %d (%d%%) (cost $%d)" ) :
                                              _( "%s: %d (%d%%) -> %d" ),
                                              trained.obj().name(), cur_level, cur_level_exercise,
                                              next_level, next_level_exercise, cost / 100 );
            add_response( text, "TALK_TRAIN_START", trained );
        }
        add_response_none( _( "Eh, never mind." ) );
    } else if( topic == "TALK_HOW_MUCH_FURTHER" ) {
        add_response_none( _( "Okay, thanks." ) );
        add_response_done( _( "Let's keep moving." ) );
    }

    if( g->u.has_trait( trait_DEBUG_MIND_CONTROL ) && !p->is_player_ally() ) {
        add_response( _( "OBEY ME!" ), "TALK_MIND_CONTROL" );
        add_response_done( _( "Bye." ) );
    }

    if( ret.empty() ) {
        add_response_done( _( "Bye." ) );
    }
}

static int parse_mod( const dialogue &d, const std::string &attribute, const int factor )
{
    player &u = *d.alpha;
    npc &p = *d.beta;
    int modifier = 0;
    if( attribute == "ANGER" ) {
        modifier = p.op_of_u.anger;
    } else if( attribute == "FEAR" ) {
        modifier = p.op_of_u.fear;
    } else if( attribute == "TRUST" ) {
        modifier = p.op_of_u.trust;
    } else if( attribute == "VALUE" ) {
        modifier = p.op_of_u.value;
    } else if( attribute == "POS_FEAR" ) {
        modifier = std::max( 0, p.op_of_u.fear );
    } else if( attribute == "AGGRESSION" ) {
        modifier = p.personality.aggression;
    } else if( attribute == "ALTRUISM" ) {
        modifier = p.personality.altruism;
    } else if( attribute == "BRAVERY" ) {
        modifier = p.personality.bravery;
    } else if( attribute == "COLLECTOR" ) {
        modifier = p.personality.collector;
    } else if( attribute == "MISSIONS" ) {
        modifier = p.assigned_missions_value() / OWED_VAL;
    } else if( attribute == "U_INTIMIDATE" ) {
        modifier = u.intimidation();
    } else if( attribute == "NPC_INTIMIDATE" ) {
        modifier = p.intimidation();
    }
    modifier *= factor;
    return modifier;
}

int talk_trial::calc_chance( const dialogue &d ) const
{
    player &u = *d.alpha;
    if( u.has_trait( trait_DEBUG_MIND_CONTROL ) ) {
        return 100;
    }
    const social_modifiers &u_mods = u.get_mutation_social_mods();

    npc &p = *d.beta;
    int chance = difficulty;
    switch( type ) {
        case NUM_TALK_TRIALS:
            dbg( D_ERROR ) << "called calc_chance with invalid talk_trial value: " << type;
            break;
        case TALK_TRIAL_LIE:
            chance += u.talk_skill() - p.talk_skill() + p.op_of_u.trust * 3;
            chance += u_mods.lie;

            //come on, who would suspect a robot of lying?
            if( u.has_bionic( bionic_id( "bio_voice" ) ) ) {
                chance += 10;
            }
            if( u.has_bionic( bionic_id( "bio_face_mask" ) ) ) {
                chance += 20;
            }
            break;
        case TALK_TRIAL_PERSUADE:
            chance += u.talk_skill() - static_cast<int>( p.talk_skill() / 2 ) +
                      p.op_of_u.trust * 2 + p.op_of_u.value;
            chance += u_mods.persuade;

            if( u.has_bionic( bionic_id( "bio_face_mask" ) ) ) {
                chance += 10;
            }
            if( u.has_bionic( bionic_id( "bio_deformity" ) ) ) {
                chance -= 50;
            }
            if( u.has_bionic( bionic_id( "bio_voice" ) ) ) {
                chance -= 20;
            }
            break;
        case TALK_TRIAL_INTIMIDATE:
            chance += u.intimidation() - p.intimidation() + p.op_of_u.fear * 2 -
                      p.personality.bravery * 2;
            chance += u_mods.intimidate;

            if( u.has_bionic( bionic_id( "bio_face_mask" ) ) ) {
                chance += 10;
            }
            if( u.has_bionic( bionic_id( "bio_armor_eyes" ) ) ) {
                chance += 10;
            }
            if( u.has_bionic( bionic_id( "bio_deformity" ) ) ) {
                chance += 20;
            }
            if( u.has_bionic( bionic_id( "bio_voice" ) ) ) {
                chance += 20;
            }
            break;
        case TALK_TRIAL_NONE:
            chance = 100;
            break;
        case TALK_TRIAL_CONDITION:
            chance = condition( d ) ? 100 : 0;
            break;
    }
    for( const auto &this_mod : modifiers ) {
        chance += parse_mod( d, this_mod.first, this_mod.second );
    }

    return std::max( 0, std::min( 100, chance ) );
}

bool talk_trial::roll( dialogue &d ) const
{
    player &u = *d.alpha;
    if( type == TALK_TRIAL_NONE || u.has_trait( trait_DEBUG_MIND_CONTROL ) ) {
        return true;
    }
    const int chance = calc_chance( d );
    const bool success = rng( 0, 99 ) < chance;
    if( success ) {
        u.practice( skill_speech, ( 100 - chance ) / 10 );
    } else {
        u.practice( skill_speech, ( 100 - chance ) / 7 );
    }
    return success;
}

int topic_category( const talk_topic &the_topic )
{
    const auto &topic = the_topic.id;
    // TODO: ideally, this would be a property of the topic itself.
    // How this works: each category has a set of topics that belong to it, each set is checked
    // for the given topic and if a set contains, the category number is returned.
    static const std::unordered_set<std::string> topic_1 = { {
            "TALK_MISSION_START", "TALK_MISSION_DESCRIBE", "TALK_MISSION_OFFER",
            "TALK_MISSION_ACCEPTED", "TALK_MISSION_REJECTED", "TALK_MISSION_ADVICE",
            "TALK_MISSION_INQUIRE", "TALK_MISSION_SUCCESS", "TALK_MISSION_SUCCESS_LIE",
            "TALK_MISSION_FAILURE", "TALK_MISSION_REWARD", "TALK_MISSION_END",
            "TALK_MISSION_DESCRIBE_URGENT"
        }
    };
    if( topic_1.count( topic ) > 0 ) {
        return 1;
    }
    static const std::unordered_set<std::string> topic_2 = { {
            "TALK_SHARE_EQUIPMENT", "TALK_GIVE_EQUIPMENT", "TALK_DENY_EQUIPMENT"
        }
    };
    if( topic_2.count( topic ) > 0 ) {
        return 2;
    }
    static const std::unordered_set<std::string> topic_3 = { {
            "TALK_SUGGEST_FOLLOW", "TALK_AGREE_FOLLOW", "TALK_DENY_FOLLOW",
        }
    };
    if( topic_3.count( topic ) > 0 ) {
        return 3;
    }
    static const std::unordered_set<std::string> topic_4 = { {
            "TALK_COMBAT_ENGAGEMENT",
        }
    };
    if( topic_4.count( topic ) > 0 ) {
        return 4;
    }
    static const std::unordered_set<std::string> topic_5 = { {
            "TALK_COMBAT_COMMANDS",
        }
    };
    if( topic_5.count( topic ) > 0 ) {
        return 5;
    }
    static const std::unordered_set<std::string> topic_6 = { {
            "TALK_TRAIN", "TALK_TRAIN_START", "TALK_TRAIN_FORCE"
        }
    };
    if( topic_6.count( topic ) > 0 ) {
        return 6;
    }
    static const std::unordered_set<std::string> topic_7 = { {
            "TALK_MISC_RULES",
        }
    };
    if( topic_7.count( topic ) > 0 ) {
        return 7;
    }
    static const std::unordered_set<std::string> topic_8 = { {
            "TALK_AIM_RULES",
        }
    };
    if( topic_8.count( topic ) > 0 ) {
        return 8;
    }
    static const std::unordered_set<std::string> topic_9 = { {
            "TALK_FRIEND", "TALK_GIVE_ITEM", "TALK_USE_ITEM",
        }
    };
    if( topic_9.count( topic ) > 0 ) {
        return 9;
    }
    static const std::unordered_set<std::string> topic_99 = { {
            "TALK_SIZE_UP", "TALK_LOOK_AT", "TALK_OPINION", "TALK_SHOUT"
        }
    };
    if( topic_99.count( topic ) > 0 ) {
        return 99;
    }
    return -1; // Not grouped with other topics
}

void parse_tags( std::string &phrase, const player &u, const player &me, const itype_id &item_type )
{
    phrase = SNIPPET.expand( remove_color_tags( phrase ) );

    size_t fa;
    size_t fb;
    std::string tag;
    do {
        fa = phrase.find( '<' );
        fb = phrase.find( '>' );
        int l = fb - fa + 1;
        if( fa != std::string::npos && fb != std::string::npos ) {
            tag = phrase.substr( fa, fb - fa + 1 );
        } else {
            return;
        }

        // Special, dynamic tags go here
        if( tag == "<yrwp>" ) {
            phrase.replace( fa, l, remove_color_tags( u.weapon.tname() ) );
        } else if( tag == "<mywp>" ) {
            if( !me.is_armed() ) {
                phrase.replace( fa, l, _( "fists" ) );
            } else {
                phrase.replace( fa, l, remove_color_tags( me.weapon.tname() ) );
            }
        } else if( tag == "<ammo>" ) {
            if( !me.weapon.is_gun() ) {
                phrase.replace( fa, l, _( "BADAMMO" ) );
            } else {
                phrase.replace( fa, l, me.weapon.ammo_current() );
            }
        } else if( tag == "<current_activity>" ) {
            std::string activity_name;
            const npc *guy = dynamic_cast<const npc *>( &me );
            if( guy->current_activity_id ) {
                activity_name = guy->current_activity_id.obj().verb().translated();
            } else {
                activity_name = _( "doing this and that" );
            }
            phrase.replace( fa, l, activity_name );
        } else if( tag == "<punc>" ) {
            switch( rng( 0, 2 ) ) {
                case 0:
                    phrase.replace( fa, l, pgettext( "punctuation", "." ) );
                    break;
                case 1:
                    phrase.replace( fa, l, pgettext( "punctuation", "..." ) );
                    break;
                case 2:
                    phrase.replace( fa, l, pgettext( "punctuation", "!" ) );
                    break;
            }
        } else if( tag == "<mypronoun>" ) {
            std::string npcstr = me.male ? pgettext( "npc", "He" ) : pgettext( "npc", "She" );
            phrase.replace( fa, l, npcstr );
        } else if( tag == "<topic_item>" ) {
            phrase.replace( fa, l, item::nname( item_type, 2 ) );
        } else if( tag == "<topic_item_price>" ) {
            item tmp( item_type );
            phrase.replace( fa, l, format_money( tmp.price( true ) ) );
        } else if( tag == "<topic_item_my_total_price>" ) {
            item tmp( item_type );
            tmp.charges = me.charges_of( item_type );
            phrase.replace( fa, l, format_money( tmp.price( true ) ) );
        } else if( tag == "<topic_item_your_total_price>" ) {
            item tmp( item_type );
            tmp.charges = u.charges_of( item_type );
            phrase.replace( fa, l, format_money( tmp.price( true ) ) );
        } else if( !tag.empty() ) {
            debugmsg( "Bad tag. '%s' (%d - %d)", tag.c_str(), fa, fb );
            phrase.replace( fa, fb - fa + 1, "????" );
        }
    } while( fa != std::string::npos && fb != std::string::npos );
}

void dialogue::add_topic( const std::string &topic_id )
{
    topic_stack.push_back( talk_topic( topic_id ) );
}

void dialogue::add_topic( const talk_topic &topic )
{
    topic_stack.push_back( topic );
}

talk_data talk_response::create_option_line( const dialogue &d, const char letter )
{
    std::string ftext;
    text = ( truefalse_condition( d ) ? truetext : falsetext ).translated();
    // dialogue w/ a % chance to work
    if( trial.type == TALK_TRIAL_NONE || trial.type == TALK_TRIAL_CONDITION ) {
        // regular dialogue
        //~ %1$c is an option letter and shouldn't be translated, %2$s is translated response text
        ftext = string_format( pgettext( "talk option", "%1$c: %2$s" ), letter, text );
    } else {
        // dialogue w/ a % chance to work
        //~ %1$c is an option letter and shouldn't be translated, %2$s is translated trial type, %3$d is a number, and %4$s is the translated response text
        ftext = string_format( pgettext( "talk option", "%1$c: [%2$s %3$d%%] %4$s" ), letter,
                               trial.name(), trial.calc_chance( d ), text );
    }
    parse_tags( ftext, *d.alpha, *d.beta, success.next_topic.item_type );

    nc_color color;
    std::set<dialogue_consequence> consequences = get_consequences( d );
    if( consequences.count( dialogue_consequence::hostile ) > 0 ) {
        color = c_red;
    } else if( text[0] == '*' || consequences.count( dialogue_consequence::helpless ) > 0 ) {
        color = c_light_red;
    } else if( text[0] == '&' || consequences.count( dialogue_consequence::action ) > 0 ) {
        color = c_green;
    } else {
        color = c_white;
    }
    talk_data results;
    results.first = color;
    results.second = ftext;
    return results;
}

std::set<dialogue_consequence> talk_response::get_consequences( const dialogue &d ) const
{
    int chance = trial.calc_chance( d );
    if( chance >= 100 ) {
        return { success.get_consequence( d ) };
    } else if( chance <= 0 ) {
        return { failure.get_consequence( d ) };
    }

    return {{ success.get_consequence( d ), failure.get_consequence( d ) }};
}

dialogue_consequence talk_effect_t::get_consequence( const dialogue &d ) const
{
    if( d.beta->op_of_u.anger + opinion.anger >= d.beta->hostile_anger_level() ) {
        return dialogue_consequence::hostile;
    }
    return guaranteed_consequence;
}

const talk_topic &special_talk( char ch )
{
    static const std::map<char, talk_topic> key_map = {{
            { 'L', talk_topic( "TALK_LOOK_AT" ) },
            { 'S', talk_topic( "TALK_SIZE_UP" ) },
            { 'O', talk_topic( "TALK_OPINION" ) },
            { 'Y', talk_topic( "TALK_SHOUT" ) },
        }
    };

    const auto iter = key_map.find( ch );
    if( iter != key_map.end() ) {
        return iter->second;
    }

    static const talk_topic no_topic = talk_topic( "TALK_NONE" );
    return no_topic;
}

talk_topic dialogue::opt( dialogue_window &d_win, const talk_topic &topic )
{
    bool text_only = d_win.text_only;
    std::string challenge = dynamic_line( topic );
    gen_responses( topic );
    // Put quotes around challenge (unless it's an action)
    if( challenge[0] != '*' && challenge[0] != '&' ) {
        std::stringstream tmp;
        tmp << "\"" << challenge << "\"";
    }

    // Parse any tags in challenge
    parse_tags( challenge, *alpha, *beta, topic.item_type );
    capitalize_letter( challenge );

    // Prepend "My Name: "
    if( challenge[0] == '&' ) {
        // No name prepended!
        challenge = challenge.substr( 1 );
    } else if( challenge[0] == '*' ) {
        challenge = string_format( pgettext( "npc does something", "%s %s" ), beta->name,
                                   challenge.substr( 1 ) );
    } else {
        challenge = string_format( pgettext( "npc says something", "%s: %s" ), beta->name,
                                   challenge );
    }

    d_win.add_history_separator();

    // Number of lines to highlight
    const size_t hilight_lines = d_win.add_to_history( challenge );

    apply_speaker_effects( topic );

    std::vector<talk_data> response_lines;
    for( size_t i = 0; i < responses.size(); i++ ) {
        response_lines.push_back( responses[i].create_option_line( *this, 'a' + i ) );
    }

    int ch = text_only ? 'a' + responses.size() - 1 : ' ';
    bool okay;
    do {
        d_win.refresh_response_display();
        do {
            d_win.display_responses( hilight_lines, response_lines, ch );
            if( !text_only ) {
                ch = inp_mngr.get_input_event().get_first_input();
            }
            auto st = special_talk( ch );
            if( st.id != "TALK_NONE" ) {
                return st;
            }
            switch( ch ) {
                // send scroll control keys back to the display window
                case KEY_DOWN:
                case KEY_NPAGE:
                case KEY_UP:
                case KEY_PPAGE:
                    continue;
                default:
                    ch -= 'a';
                    break;
            }
        } while( ( ch < 0 || ch >= static_cast<int>( responses.size() ) ) );
        okay = true;
        std::set<dialogue_consequence> consequences = responses[ch].get_consequences( *this );
        if( consequences.count( dialogue_consequence::hostile ) > 0 ) {
            okay = query_yn( _( "You may be attacked! Proceed?" ) );
        } else if( consequences.count( dialogue_consequence::helpless ) > 0 ) {
            okay = query_yn( _( "You'll be helpless! Proceed?" ) );
        }
    } while( !okay );
    d_win.add_history_separator();

    talk_response chosen = responses[ch];
    std::string response_printed = string_format( pgettext( "you say something", "You: %s" ),
                                   response_lines[ch].second.substr( 3 ) );
    d_win.add_to_history( response_printed );

    if( chosen.mission_selected != nullptr ) {
        beta->chatbin.mission_selected = chosen.mission_selected;
    }

    // We can't set both skill and style or training will bug out
    // TODO: Allow setting both skill and style
    if( chosen.skill ) {
        beta->chatbin.skill = chosen.skill;
        beta->chatbin.style = matype_id::NULL_ID();
        beta->chatbin.dialogue_spell = spell_id();
    } else if( chosen.style ) {
        beta->chatbin.style = chosen.style;
        beta->chatbin.skill = skill_id::NULL_ID();
        beta->chatbin.dialogue_spell = spell_id();
    } else if( chosen.dialogue_spell != spell_id() ) {
        beta->chatbin.style = matype_id::NULL_ID();
        beta->chatbin.skill = skill_id::NULL_ID();
        beta->chatbin.dialogue_spell = chosen.dialogue_spell;
    }
    const bool success = chosen.trial.roll( *this );
    const auto &effects = success ? chosen.success : chosen.failure;
    return effects.apply( *this );
}

talk_trial::talk_trial( JsonObject jo )
{
    static const std::unordered_map<std::string, talk_trial_type> types_map = { {
#define WRAP(value) { #value, TALK_TRIAL_##value }
            WRAP( NONE ),
            WRAP( LIE ),
            WRAP( PERSUADE ),
            WRAP( INTIMIDATE ),
            WRAP( CONDITION )
#undef WRAP
        }
    };
    const auto iter = types_map.find( jo.get_string( "type", "NONE" ) );
    if( iter == types_map.end() ) {
        jo.throw_error( "invalid talk trial type", "type" );
    }
    type = iter->second;
    if( !( type == TALK_TRIAL_NONE || type == TALK_TRIAL_CONDITION ) ) {
        difficulty = jo.get_int( "difficulty" );
    }

    read_condition<dialogue>( jo, "condition", condition, false );

    if( jo.has_array( "mod" ) ) {
        JsonArray ja = jo.get_array( "mod" );
        while( ja.has_more() ) {
            JsonArray jmod = ja.next_array();
            trial_mod this_modifier;
            this_modifier.first = jmod.next_string();
            this_modifier.second = jmod.next_int();
            modifiers.push_back( this_modifier );
        }
    }
}

static talk_topic load_inline_topic( JsonObject jo )
{
    const std::string id = jo.get_string( "id" );
    json_talk_topics[id].load( jo );
    return talk_topic( id );
}

talk_effect_fun_t::talk_effect_fun_t( talkfunction_ptr ptr )
{
    function = [ptr]( const dialogue & d ) {
        npc &p = *d.beta;
        ptr( p );
    };
}

talk_effect_fun_t::talk_effect_fun_t( std::function<void( npc &p )> ptr )
{
    function = [ptr]( const dialogue & d ) {
        npc &p = *d.beta;
        ptr( p );
    };
}

talk_effect_fun_t::talk_effect_fun_t( std::function<void( const dialogue &d )> fun )
{
    function = [fun]( const dialogue & d ) {
        fun( d );
    };
}

void talk_effect_fun_t::set_companion_mission( const std::string &role_id )
{
    function = [role_id]( const dialogue & d ) {
        npc &p = *d.beta;
        p.companion_mission_role_id = role_id;
        talk_function::companion_mission( p );
    };
}

void talk_effect_fun_t::set_add_effect( JsonObject jo, const std::string &member, bool is_npc )
{
    std::string new_effect = jo.get_string( member );
    bool permanent = false;
    time_duration duration = 1000_turns;
    if( jo.has_string( "duration" ) ) {
        const std::string dur_string = jo.get_string( "duration" );
        if( dur_string == "PERMANENT" ) {
            permanent = true;
        } else if( !dur_string.empty() && std::stoi( dur_string ) > 0 ) {
            duration = time_duration::from_turns( std::stoi( dur_string ) );
        }
    } else {
        duration = time_duration::from_turns( jo.get_int( "duration" ) );
    }
    function = [is_npc, new_effect, duration, permanent]( const dialogue & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        actor->add_effect( efftype_id( new_effect ), duration, num_bp, permanent );
    };
}

void talk_effect_fun_t::set_remove_effect( JsonObject jo, const std::string &member, bool is_npc )
{
    std::string old_effect = jo.get_string( member );
    function = [is_npc, old_effect]( const dialogue & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        actor->remove_effect( efftype_id( old_effect ), num_bp );
    };
}

void talk_effect_fun_t::set_add_trait( JsonObject jo, const std::string &member, bool is_npc )
{
    std::string new_trait = jo.get_string( member );
    function = [is_npc, new_trait]( const dialogue & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        actor->set_mutation( trait_id( new_trait ) );
    };
}

void talk_effect_fun_t::set_remove_trait( JsonObject jo, const std::string &member, bool is_npc )
{
    std::string old_trait = jo.get_string( member );
    function = [is_npc, old_trait]( const dialogue & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        actor->unset_mutation( trait_id( old_trait ) );
    };
}

void talk_effect_fun_t::set_add_var( JsonObject jo, const std::string &member, bool is_npc )
{
    const std::string var_name = get_talk_varname( jo, member );
    const std::string &value = jo.get_string( "value" );
    function = [is_npc, var_name, value]( const dialogue & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        actor->set_value( var_name, value );
    };
}

void talk_effect_fun_t::set_remove_var( JsonObject jo, const std::string &member, bool is_npc )
{
    const std::string var_name = get_talk_varname( jo, member, false );
    function = [is_npc, var_name]( const dialogue & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        actor->remove_value( var_name );
    };
}

void talk_effect_fun_t::set_adjust_var( JsonObject jo, const std::string &member, bool is_npc )
{
    const std::string var_name = get_talk_varname( jo, member, false );
    const int value = jo.get_int( "adjustment" );
    function = [is_npc, var_name, value]( const dialogue & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }

        int adjusted_value = value;

        const std::string &var = actor->get_value( var_name );
        if( !var.empty() ) {
            adjusted_value += std::stoi( var );
        }

        actor->set_value( var_name, std::to_string( adjusted_value ) );
    };
}

void talk_effect_fun_t::set_u_buy_item( const std::string &item_name, int cost, int count,
                                        const std::string &container_name )
{
    function = [item_name, cost, count, container_name]( const dialogue & d ) {
        npc &p = *d.beta;
        player &u = *d.alpha;
        if( !npc_trading::pay_npc( p, cost ) ) {
            popup( _( "You can't afford it!" ) );
            return;
        }
        if( container_name.empty() ) {
            item new_item = item( item_name, calendar::turn, 1 );
            if( new_item.count_by_charges() ) {
                new_item.mod_charges( count - 1 );
                u.i_add( new_item );
            } else {
                for( int i_cnt = 0; i_cnt < count; i_cnt++ ) {
                    u.i_add( new_item );
                }
            }
            if( count == 1 ) {
                //~ %1%s is the NPC name, %2$s is an item
                popup( _( "%1$s gives you a %2$s." ), p.name, new_item.tname() );
            } else {
                //~ %1%s is the NPC name, %2$d is a number of items, %3$s are items
                popup( _( "%1$s gives you %2$d %3$s." ), p.name, count, new_item.tname() );
            }
        } else {
            item container( container_name, calendar::turn );
            container.emplace_back( item_name, calendar::turn, count );
            u.i_add( container );
            //~ %1%s is the NPC name, %2$s is an item
            popup( _( "%1$s gives you a %2$s." ), p.name, container.tname() );
        }
    };

    // Update structure used by mission descriptions.
    if( cost <= 0 ) {
        likely_rewards.push_back( std::pair<int, std::string>( count, item_name ) );
    }
}

void talk_effect_fun_t::set_u_sell_item( const std::string &item_name, int cost, int count )
{
    function = [item_name, cost, count]( const dialogue & d ) {
        npc &p = *d.beta;
        player &u = *d.alpha;
        item old_item = item( item_name, calendar::turn, 1 );
        if( u.has_charges( item_name, count ) ) {
            u.use_charges( item_name, count );
        } else if( u.has_amount( item_name, count ) ) {
            u.use_amount( item_name, count );
        } else {
            //~ %1$s is a translated item name
            popup( _( "You don't have a %1$s!" ), old_item.tname() );
            return;
        }
        if( old_item.count_by_charges() ) {
            old_item.mod_charges( count - 1 );
            p.i_add( old_item );
        } else {
            for( int i_cnt = 0; i_cnt < count; i_cnt++ ) {
                p.i_add( old_item );
            }
        }

        if( count == 1 ) {
            //~ %1%s is the NPC name, %2$s is an item
            popup( _( "You give %1$s a %2$s." ), p.name, old_item.tname() );
        } else {
            //~ %1%s is the NPC name, %2$d is a number of items, %3$s are items
            popup( _( "You give %1$s %2$d %3$s." ), p.name, count, old_item.tname() );
        }
        p.op_of_u.owed += cost;
    };
}

void talk_effect_fun_t::set_consume_item( JsonObject jo, const std::string &member, int count,
        bool is_npc )
{
    const std::string &item_name = jo.get_string( member );
    function = [is_npc, item_name, count]( const dialogue & d ) {
        // this is stupid, but I couldn't get the assignment to work
        const auto consume_item = [&]( player & p, const std::string & item_name, int count ) {
            item old_item( item_name );
            if( p.has_charges( item_name, count ) ) {
                p.use_charges( item_name, count );
            } else if( p.has_amount( item_name, count ) ) {
                p.use_amount( item_name, count );
            } else {
                //~ %1%s is the "You" or the NPC name, %2$s are a translated item name
                popup( _( "%1$s doesn't have a %2$s!" ), p.disp_name(), old_item.tname() );
            }
        };
        if( is_npc ) {
            consume_item( *d.beta, item_name, count );
        } else {
            consume_item( *d.alpha, item_name, count );
        }
    };
}

void talk_effect_fun_t::set_remove_item_with( JsonObject jo, const std::string &member,
        bool is_npc )
{
    const std::string &item_name = jo.get_string( member );
    function = [is_npc, item_name]( const dialogue & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        itype_id item_id = itype_id( item_name );
        actor->remove_items_with( [item_id]( const item & it ) {
            return it.typeId() == item_id;
        } );
    };
}

void talk_effect_fun_t::set_u_spend_cash( int amount )
{
    function = [amount]( const dialogue & d ) {
        npc &np = *d.beta;
        npc_trading::pay_npc( np, amount );
    };
}

void talk_effect_fun_t::set_npc_change_faction( const std::string &faction_name )
{
    function = [faction_name]( const dialogue & d ) {
        npc &p = *d.beta;
        p.set_fac( faction_id( faction_name ) );
    };
}

void talk_effect_fun_t::set_npc_change_class( const std::string &class_name )
{
    function = [class_name]( const dialogue & d ) {
        npc &p = *d.beta;
        p.myclass = npc_class_id( class_name );
    };
}

void talk_effect_fun_t::set_change_faction_rep( int rep_change )
{
    function = [rep_change]( const dialogue & d ) {
        npc &p = *d.beta;
        if( p.get_faction()->id != faction_id( "no_faction" ) ) {
            p.get_faction()->likes_u += rep_change;
            p.get_faction()->respects_u += rep_change;
        }
    };
}

void talk_effect_fun_t::set_add_debt( const std::vector<trial_mod> &debt_modifiers )
{
    function = [debt_modifiers]( const dialogue & d ) {
        int debt = 0;
        for( const trial_mod &this_mod : debt_modifiers ) {
            if( this_mod.first == "TOTAL" ) {
                debt *= this_mod.second;
            } else {
                debt += parse_mod( d, this_mod.first, this_mod.second );
            }
        }
        d.beta->op_of_u += npc_opinion( 0, 0, 0, 0, debt );
    };
}

void talk_effect_fun_t::set_toggle_npc_rule( const std::string &rule )
{
    function = [rule]( const dialogue & d ) {
        auto toggle = ally_rule_strs.find( rule );
        if( toggle == ally_rule_strs.end() ) {
            return;
        }
        d.beta->rules.toggle_flag( toggle->second.rule );
        d.beta->wield_better_weapon();
    };
}

void talk_effect_fun_t::set_set_npc_rule( const std::string &rule )
{
    function = [rule]( const dialogue & d ) {
        auto flag = ally_rule_strs.find( rule );
        if( flag == ally_rule_strs.end() ) {
            return;
        }
        d.beta->rules.set_flag( flag->second.rule );
        d.beta->wield_better_weapon();
    };
}

void talk_effect_fun_t::set_clear_npc_rule( const std::string &rule )
{
    function = [rule]( const dialogue & d ) {
        auto flag = ally_rule_strs.find( rule );
        if( flag == ally_rule_strs.end() ) {
            return;
        }
        d.beta->rules.clear_flag( flag->second.rule );
        d.beta->wield_better_weapon();
    };
}

void talk_effect_fun_t::set_npc_engagement_rule( const std::string &setting )
{
    function = [setting]( const dialogue & d ) {
        auto rule = combat_engagement_strs.find( setting );
        if( rule != combat_engagement_strs.end() ) {
            d.beta->rules.engagement = rule->second;
        }
    };
}

void talk_effect_fun_t::set_npc_aim_rule( const std::string &setting )
{
    function = [setting]( const dialogue & d ) {
        auto rule = aim_rule_strs.find( setting );
        if( rule != aim_rule_strs.end() ) {
            d.beta->rules.aim = rule->second;
        }
    };
}

void talk_effect_fun_t::set_npc_cbm_reserve_rule( const std::string &setting )
{
    function = [setting]( const dialogue & d ) {
        auto rule = cbm_reserve_strs.find( setting );
        if( rule != cbm_reserve_strs.end() ) {
            d.beta->rules.cbm_reserve = rule->second;
        }
    };
}

void talk_effect_fun_t::set_npc_cbm_recharge_rule( const std::string &setting )
{
    function = [setting]( const dialogue & d ) {
        auto rule = cbm_recharge_strs.find( setting );
        if( rule != cbm_recharge_strs.end() ) {
            d.beta->rules.cbm_recharge = rule->second;
        }
    };
}

void talk_effect_fun_t::set_mapgen_update( JsonObject jo, const std::string &member )
{
    mission_target_params target_params = mission_util::parse_mission_om_target( jo );
    std::vector<std::string> update_ids;

    if( jo.has_string( member ) ) {
        update_ids.emplace_back( jo.get_string( member ) );
    } else if( jo.has_array( member ) ) {
        JsonArray ja = jo.get_array( member );
        while( ja.has_more() ) {
            update_ids.emplace_back( ja.next_string() );
        }
    }

    function = [target_params, update_ids]( const dialogue & d ) {
        mission_target_params update_params = target_params;
        update_params.guy = d.beta;
        const tripoint omt_pos = mission_util::get_om_terrain_pos( update_params );
        for( const std::string mapgen_update_id : update_ids ) {
            run_mapgen_update_func( mapgen_update_id, omt_pos, d.beta->chatbin.mission_selected );
        }
    };
}

void talk_effect_fun_t::set_bulk_trade_accept( bool is_trade, bool is_npc )
{
    function = [is_trade, is_npc]( const dialogue & d ) {
        player *seller = d.alpha;
        player *buyer = dynamic_cast<player *>( d.beta );
        if( is_npc ) {
            seller = dynamic_cast<player *>( d.beta );
            buyer = d.alpha;
        }
        int seller_has = seller->charges_of( d.cur_item );
        item tmp( d.cur_item );
        tmp.charges = seller_has;
        if( is_trade ) {
            int price = tmp.price( true ) * ( is_npc ? -1 : 1 ) + d.beta->op_of_u.owed;
            if( d.beta->get_faction() && !d.beta->get_faction()->currency.empty() ) {
                const itype_id &pay_in = d.beta->get_faction()->currency;
                item pay( pay_in );
                if( d.beta->value( pay ) > 0 ) {
                    int required = price / d.beta->value( pay );
                    int buyer_has = required;
                    if( is_npc ) {
                        buyer_has = std::min( buyer_has, buyer->charges_of( pay_in ) );
                        buyer->use_charges( pay_in, buyer_has );
                    } else {
                        if( buyer_has == 1 ) {
                            //~ %1%s is the NPC name, %2$s is an item
                            popup( _( "%1$s gives you a %2$s." ), d.beta->disp_name(),
                                   pay.tname() );
                        } else if( buyer_has > 1 ) {
                            //~ %1%s is the NPC name, %2$d is a number of items, %3$s are items
                            popup( _( "%1$s gives you %2$d %3$s." ), d.beta->disp_name(), buyer_has,
                                   pay.tname() );
                        }
                    }
                    for( int i = 0; i < buyer_has; i++ ) {
                        seller->i_add( pay );
                        price -= d.beta->value( pay );
                    }
                }
                d.beta->op_of_u.owed += price;
            }
        }
        seller->use_charges( d.cur_item, seller_has );
        buyer->i_add( tmp );
    };
}

void talk_effect_fun_t::set_npc_gets_item( bool to_use )
{
    function = [to_use]( const dialogue & d ) {
        d.reason = give_item_to( *( d.beta ), to_use, !to_use );
    };
}

void talk_effect_fun_t::set_add_mission( const std::string mission_id )
{
    function = [mission_id]( const dialogue & d ) {
        npc &p = *d.beta;
        mission *miss = mission::reserve_new( mission_type_id( mission_id ), p.getID() );
        miss->assign( g->u );
        p.chatbin.missions_assigned.push_back( miss );
    };
}

const std::vector<std::pair<int, std::string>> &talk_effect_fun_t::get_likely_rewards() const
{
    return likely_rewards;
}

void talk_effect_fun_t::set_u_buy_monster( const std::string &monster_type_id, int cost, int count,
        bool pacified, const translation &name )
{
    function = [monster_type_id, cost, count, pacified, name]( const dialogue & d ) {
        npc &p = *d.beta;
        player &u = *d.alpha;
        if( !npc_trading::pay_npc( p, cost ) ) {
            popup( _( "You can't afford it!" ) );
            return;
        }

        const mtype_id mtype( monster_type_id );
        const efftype_id effect_pet( "pet" );
        const efftype_id effect_pacified( "pacified" );

        for( int i = 0; i < count; i++ ) {
            monster *const mon_ptr = g->place_critter_around( mtype, u.pos(), 3 );
            if( !mon_ptr ) {
                add_msg( m_debug, "Cannot place u_buy_monster, no valid placement locations." );
                break;
            }
            monster &tmp = *mon_ptr;
            // Our monster is always a pet.
            tmp.friendly = -1;
            tmp.add_effect( effect_pet, 1_turns, num_bp, true );

            if( pacified ) {
                tmp.add_effect( effect_pacified, 1_turns, num_bp, true );
            }

            if( !name.empty() ) {
                tmp.unique_name = name.translated();
            }

        }

        if( name.empty() ) {
            popup( _( "%1$s gives you %2$d %3$s." ), p.name, count, mtype.obj().nname( count ) );
        } else {
            popup( _( "%1$s gives you %2$s." ), p.name, name );
        }
    };
}

void talk_effect_fun_t::set_u_learn_recipe( const std::string &learned_recipe_id )
{
    function = [learned_recipe_id]( const dialogue & d ) {
        const recipe &r = recipe_id( learned_recipe_id ).obj();
        d.alpha->learn_recipe( &r );
        popup( _( "You learn how to craft %s." ), r.result_name() );
    };
}

void talk_effect_t::set_effect_consequence( const talk_effect_fun_t &fun, dialogue_consequence con )
{
    effects.push_back( fun );
    guaranteed_consequence = std::max( guaranteed_consequence, con );
}

void talk_effect_t::set_effect_consequence( std::function<void( npc &p )> ptr,
        dialogue_consequence con )
{
    talk_effect_fun_t npctalk_setter( ptr );
    set_effect_consequence( npctalk_setter, con );
}

void talk_effect_t::set_effect( const talk_effect_fun_t &fun )
{
    effects.push_back( fun );
    guaranteed_consequence = std::max( guaranteed_consequence, dialogue_consequence::none );
}

void talk_effect_t::set_effect( talkfunction_ptr ptr )
{
    talk_effect_fun_t npctalk_setter( ptr );
    dialogue_consequence response;
    if( ptr == &talk_function::hostile ) {
        response = dialogue_consequence::hostile;
    } else if( ptr == &talk_function::player_weapon_drop ||
               ptr == &talk_function::player_weapon_away ||
               ptr == &talk_function::start_mugging ) {
        response = dialogue_consequence::helpless;
    } else {
        response = dialogue_consequence::none;
    }
    set_effect_consequence( npctalk_setter, response );
}

talk_topic talk_effect_t::apply( dialogue &d ) const
{
    // Need to get a reference to the mission before effects are applied, because effects can remove the mission
    mission *miss = d.beta->chatbin.mission_selected;

    for( const talk_effect_fun_t &effect : effects ) {
        effect( d );
    }
    d.beta->op_of_u += opinion;
    if( miss && ( mission_opinion.trust || mission_opinion.fear ||
                  mission_opinion.value || mission_opinion.anger ) ) {
        int m_value = npc_trading::cash_to_favor( *d.beta, miss->get_value() );
        npc_opinion mod = npc_opinion( mission_opinion.trust ?
                                       m_value / mission_opinion.trust : 0,
                                       mission_opinion.fear ?
                                       m_value / mission_opinion.fear : 0,
                                       mission_opinion.value ?
                                       m_value / mission_opinion.value : 0,
                                       mission_opinion.anger ?
                                       m_value / mission_opinion.anger : 0, 0 );
        d.beta->op_of_u += mod;
    }
    if( d.beta->turned_hostile() ) {
        d.beta->make_angry();
        return talk_topic( "TALK_DONE" );
    }

    // TODO: this is a hack, it should be in clear_mission or so, but those functions have
    // no access to the dialogue object.
    auto &ma = d.missions_assigned;
    ma.clear();
    // Update the missions we can talk about (must only be current, non-complete ones)
    for( auto &mission : d.beta->chatbin.missions_assigned ) {
        if( mission->get_assigned_player_id() == d.alpha->getID() ) {
            ma.push_back( mission );
        }
    }

    return next_topic;
}

talk_effect_t::talk_effect_t( JsonObject jo )
{
    load_effect( jo );
    if( jo.has_object( "topic" ) ) {
        next_topic = load_inline_topic( jo.get_object( "topic" ) );
    } else if( jo.has_string( "topic" ) ) {
        next_topic = talk_topic( jo.get_string( "topic" ) );
    }
}

void talk_effect_t::parse_sub_effect( JsonObject jo )
{
    talk_effect_fun_t subeffect_fun;
    const bool is_npc = true;
    if( jo.has_string( "companion_mission" ) ) {
        std::string role_id = jo.get_string( "companion_mission" );
        subeffect_fun.set_companion_mission( role_id );
    } else if( jo.has_string( "u_add_effect" ) ) {
        subeffect_fun.set_add_effect( jo, "u_add_effect" );
    } else if( jo.has_string( "npc_add_effect" ) ) {
        subeffect_fun.set_add_effect( jo, "npc_add_effect", is_npc );
    } else if( jo.has_string( "u_lose_effect" ) ) {
        subeffect_fun.set_remove_effect( jo, "u_lose_effect" );
    } else if( jo.has_string( "npc_lose_effect" ) ) {
        subeffect_fun.set_remove_effect( jo, "npc_lose_effect", is_npc );
    } else if( jo.has_string( "u_add_var" ) ) {
        subeffect_fun.set_add_var( jo, "u_add_var" );
    } else if( jo.has_string( "npc_add_var" ) ) {
        subeffect_fun.set_add_var( jo, "npc_add_var", is_npc );
    } else if( jo.has_string( "u_lose_var" ) ) {
        subeffect_fun.set_remove_var( jo, "u_lose_var" );
    } else if( jo.has_string( "npc_lose_var" ) ) {
        subeffect_fun.set_remove_var( jo, "npc_lose_var", is_npc );
    } else if( jo.has_string( "u_adjust_var" ) ) {
        subeffect_fun.set_adjust_var( jo, "u_adjust_var" );
    } else if( jo.has_string( "npc_adjust_var" ) ) {
        subeffect_fun.set_adjust_var( jo, "npc_adjust_var", is_npc );
    } else if( jo.has_string( "u_add_trait" ) ) {
        subeffect_fun.set_add_trait( jo, "u_add_trait" );
    } else if( jo.has_string( "npc_add_trait" ) ) {
        subeffect_fun.set_add_trait( jo, "npc_add_trait", is_npc );
    } else if( jo.has_string( "u_lose_trait" ) ) {
        subeffect_fun.set_remove_trait( jo, "u_lose_trait" );
    } else if( jo.has_string( "npc_lose_trait" ) ) {
        subeffect_fun.set_remove_trait( jo, "npc_lose_trait", is_npc );
    } else if( jo.has_int( "u_spend_cash" ) ) {
        int cash_change = jo.get_int( "u_spend_cash" );
        subeffect_fun.set_u_spend_cash( cash_change );
    } else if( jo.has_string( "u_sell_item" ) || jo.has_string( "u_buy_item" ) ||
               jo.has_string( "u_consume_item" ) || jo.has_string( "npc_consume_item" ) ||
               jo.has_string( "u_remove_item_with" ) || jo.has_string( "npc_remove_item_with" ) ) {
        int cost = 0;
        if( jo.has_int( "cost" ) ) {
            cost = jo.get_int( "cost" );
        }
        int count = 1;
        if( jo.has_int( "count" ) ) {
            count = jo.get_int( "count" );
        }
        std::string container_name;
        if( jo.has_string( "container" ) ) {
            container_name = jo.get_string( "container" );
        }
        if( jo.has_string( "u_sell_item" ) ) {
            const std::string &item_name = jo.get_string( "u_sell_item" );
            subeffect_fun.set_u_sell_item( item_name, cost, count );
        } else if( jo.has_string( "u_buy_item" ) ) {
            const std::string &item_name = jo.get_string( "u_buy_item" );
            subeffect_fun.set_u_buy_item( item_name, cost, count, container_name );
        } else if( jo.has_string( "u_consume_item" ) ) {
            subeffect_fun.set_consume_item( jo, "u_consume_item", count );
        } else if( jo.has_string( "npc_consume_item" ) ) {
            subeffect_fun.set_consume_item( jo, "npc_consume_item", count, is_npc );
        } else if( jo.has_string( "u_remove_item_with" ) ) {
            subeffect_fun.set_remove_item_with( jo, "u_remove_item_with" );
        } else if( jo.has_string( "npc_remove_item_with" ) ) {
            subeffect_fun.set_remove_item_with( jo, "npc_remove_item_with", is_npc );
        }
    } else if( jo.has_string( "npc_change_class" ) ) {
        std::string class_name = jo.get_string( "npc_change_class" );
        subeffect_fun.set_npc_change_class( class_name );
    } else if( jo.has_string( "add_mission" ) ) {
        std::string mission_id = jo.get_string( "add_mission" );
        subeffect_fun.set_add_mission( mission_id );
    } else if( jo.has_string( "npc_change_faction" ) ) {
        std::string faction_name = jo.get_string( "npc_change_faction" );
        subeffect_fun.set_npc_change_faction( faction_name );
    } else if( jo.has_int( "u_faction_rep" ) ) {
        int faction_rep = jo.get_int( "u_faction_rep" );
        subeffect_fun.set_change_faction_rep( faction_rep );
    } else if( jo.has_array( "add_debt" ) ) {
        std::vector<trial_mod> debt_modifiers;
        JsonArray ja = jo.get_array( "add_debt" );
        while( ja.has_more() ) {
            JsonArray jmod = ja.next_array();
            trial_mod this_modifier;
            this_modifier.first = jmod.next_string();
            this_modifier.second = jmod.next_int();
            debt_modifiers.push_back( this_modifier );
        }
        subeffect_fun.set_add_debt( debt_modifiers );
    } else if( jo.has_string( "toggle_npc_rule" ) ) {
        const std::string rule = jo.get_string( "toggle_npc_rule" );
        subeffect_fun.set_toggle_npc_rule( rule );
    } else if( jo.has_string( "set_npc_rule" ) ) {
        const std::string rule = jo.get_string( "set_npc_rule" );
        subeffect_fun.set_set_npc_rule( rule );
    } else if( jo.has_string( "clear_npc_rule" ) ) {
        const std::string rule = jo.get_string( "clear_npc_rule" );
        subeffect_fun.set_clear_npc_rule( rule );
    } else if( jo.has_string( "set_npc_engagement_rule" ) ) {
        const std::string setting = jo.get_string( "set_npc_engagement_rule" );
        subeffect_fun.set_npc_engagement_rule( setting );
    } else if( jo.has_string( "set_npc_aim_rule" ) ) {
        const std::string setting = jo.get_string( "set_npc_aim_rule" );
        subeffect_fun.set_npc_aim_rule( setting );
    } else if( jo.has_string( "set_npc_cbm_reserve_rule" ) ) {
        const std::string setting = jo.get_string( "set_npc_cbm_reserve_rule" );
        subeffect_fun.set_npc_cbm_reserve_rule( setting );
    } else if( jo.has_string( "set_npc_cbm_recharge_rule" ) ) {
        const std::string setting = jo.get_string( "set_npc_cbm_recharge_rule" );
        subeffect_fun.set_npc_cbm_recharge_rule( setting );
    } else if( jo.has_member( "mapgen_update" ) ) {
        subeffect_fun.set_mapgen_update( jo, "mapgen_update" );
    } else if( jo.has_string( "u_buy_monster" ) ) {
        const std::string &monster_type_id = jo.get_string( "u_buy_monster" );
        const int cost = jo.get_int( "cost", 0 );
        const int count = jo.get_int( "count", 1 );
        const bool pacified = jo.get_bool( "pacified", false );
        translation name;
        jo.read( "name", name );
        subeffect_fun.set_u_buy_monster( monster_type_id, cost, count, pacified, name );
    } else if( jo.has_string( "u_learn_recipe" ) ) {
        const std::string recipe_id = jo.get_string( "u_learn_recipe" );
        subeffect_fun.set_u_learn_recipe( recipe_id );
    } else {
        jo.throw_error( "invalid sub effect syntax :" + jo.str() );
    }
    set_effect( subeffect_fun );
}

void talk_effect_t::parse_string_effect( const std::string &effect_id, JsonObject &jo )
{
    static const std::unordered_map<std::string, void( * )( npc & )> static_functions_map = {
        {
#define WRAP( function ) { #function, &talk_function::function }
            WRAP( assign_mission ),
            WRAP( mission_success ),
            WRAP( mission_failure ),
            WRAP( clear_mission ),
            WRAP( mission_reward ),
            WRAP( start_trade ),
            WRAP( sort_loot ),
            WRAP( find_mount ),
            WRAP( dismount ),
            WRAP( do_chop_plank ),
            WRAP( do_vehicle_deconstruct ),
            WRAP( do_vehicle_repair ),
            WRAP( do_chop_trees ),
            WRAP( do_fishing ),
            WRAP( do_construction ),
            WRAP( do_read ),
            WRAP( do_butcher ),
            WRAP( do_farming ),
            WRAP( assign_guard ),
            WRAP( stop_guard ),
            WRAP( start_camp ),
            WRAP( buy_cow ),
            WRAP( buy_chicken ),
            WRAP( buy_horse ),
            WRAP( recover_camp ),
            WRAP( remove_overseer ),
            WRAP( basecamp_mission ),
            WRAP( wake_up ),
            WRAP( reveal_stats ),
            WRAP( end_conversation ),
            WRAP( insult_combat ),
            WRAP( give_equipment ),
            WRAP( give_aid ),
            WRAP( give_all_aid ),
            WRAP( barber_beard ),
            WRAP( barber_hair ),
            WRAP( buy_haircut ),
            WRAP( buy_shave ),
            WRAP( morale_chat ),
            WRAP( morale_chat_activity ),
            WRAP( buy_10_logs ),
            WRAP( buy_100_logs ),
            WRAP( bionic_install ),
            WRAP( bionic_remove ),
            WRAP( follow ),
            WRAP( follow_only ),
            WRAP( deny_follow ),
            WRAP( deny_lead ),
            WRAP( deny_equipment ),
            WRAP( deny_train ),
            WRAP( deny_personal_info ),
            WRAP( hostile ),
            WRAP( flee ),
            WRAP( leave ),
            WRAP( stop_following ),
            WRAP( revert_activity ),
            WRAP( goto_location ),
            WRAP( stranger_neutral ),
            WRAP( start_mugging ),
            WRAP( player_leaving ),
            WRAP( drop_weapon ),
            WRAP( drop_stolen_item ),
            WRAP( remove_stolen_status ),
            WRAP( player_weapon_away ),
            WRAP( player_weapon_drop ),
            WRAP( lead_to_safety ),
            WRAP( start_training ),
            WRAP( copy_npc_rules ),
            WRAP( set_npc_pickup ),
            WRAP( npc_die ),
            WRAP( npc_thankful ),
            WRAP( clear_overrides ),
            WRAP( nothing )
#undef WRAP
        }
    };
    const auto iter = static_functions_map.find( effect_id );
    if( iter != static_functions_map.end() ) {
        set_effect( iter->second );
        return;
    }

    talk_effect_fun_t subeffect_fun;
    if( effect_id == "u_bulk_trade_accept" || effect_id == "npc_bulk_trade_accept" ||
        effect_id == "u_bulk_donate" || effect_id == "npc_bulk_donate" ) {
        bool is_npc = effect_id == "npc_bulk_trade_accept" || effect_id == "npc_bulk_donate";
        bool is_trade = effect_id == "u_bulk_trade_accept" || effect_id == "npc_bulk_trade_accept";
        subeffect_fun.set_bulk_trade_accept( is_trade, is_npc );
        set_effect( subeffect_fun );
        return;
    }

    if( effect_id == "npc_gets_item" || effect_id == "npc_gets_item_to_use" ) {
        bool to_use = effect_id == "npc_gets_item_to_use";
        subeffect_fun.set_npc_gets_item( to_use );
        set_effect( subeffect_fun );
        return;
    }

    jo.throw_error( "unknown effect string", effect_id );
}

void talk_effect_t::load_effect( JsonObject &jo )
{
    if( jo.has_member( "opinion" ) ) {
        JsonIn *ji = jo.get_raw( "opinion" );
        // Same format as when saving a game (-:
        opinion.deserialize( *ji );
    }
    if( jo.has_member( "mission_opinion" ) ) {
        JsonIn *ji = jo.get_raw( "mission_opinion" );
        // Same format as when saving a game (-:
        mission_opinion.deserialize( *ji );
    }
    static const std::string member_name( "effect" );
    if( !jo.has_member( member_name ) ) {
        return;
    } else if( jo.has_string( member_name ) ) {
        const std::string type = jo.get_string( member_name );
        parse_string_effect( type, jo );
    } else if( jo.has_object( member_name ) ) {
        JsonObject sub_effect = jo.get_object( member_name );
        parse_sub_effect( sub_effect );
    } else if( jo.has_array( member_name ) ) {
        JsonArray ja = jo.get_array( member_name );
        while( ja.has_more() ) {
            if( ja.test_string() ) {
                const std::string type = ja.next_string();
                parse_string_effect( type, jo );
            } else if( ja.test_object() ) {
                JsonObject sub_effect = ja.next_object();
                parse_sub_effect( sub_effect );
            } else {
                jo.throw_error( "invalid effect array syntax", member_name );
            }
        }
    } else {
        jo.throw_error( "invalid effect syntax", member_name );
    }
}

talk_response::talk_response()
{
    truefalse_condition = []( const dialogue & ) {
        return true;
    };
    mission_selected = nullptr;
    skill = skill_id::NULL_ID();
    style = matype_id::NULL_ID();
    dialogue_spell = spell_id();
}

talk_response::talk_response( JsonObject jo )
{
    if( jo.has_member( "truefalsetext" ) ) {
        JsonObject truefalse_jo = jo.get_object( "truefalsetext" );
        read_condition<dialogue>( truefalse_jo, "condition", truefalse_condition, true );
        truefalse_jo.read( "true", truetext );
        truefalse_jo.read( "false", falsetext );
    } else {
        jo.read( "text", truetext );
        truefalse_condition = []( const dialogue & ) {
            return true;
        };
    }
    if( jo.has_member( "trial" ) ) {
        trial = talk_trial( jo.get_object( "trial" ) );
    }
    if( jo.has_member( "success" ) ) {
        success = talk_effect_t( jo.get_object( "success" ) );
    } else if( jo.has_string( "topic" ) ) {
        // This is for simple topic switching without a possible failure
        success.next_topic = talk_topic( jo.get_string( "topic" ) );
        success.load_effect( jo );
    } else if( jo.has_object( "topic" ) ) {
        success.next_topic = load_inline_topic( jo.get_object( "topic" ) );
    }
    if( trial && !jo.has_member( "failure" ) ) {
        jo.throw_error( "the failure effect is mandatory if a talk_trial has been defined" );
    }
    if( jo.has_member( "failure" ) ) {
        failure = talk_effect_t( jo.get_object( "failure" ) );
    }

    // TODO: mission_selected
    // TODO: skill
    // TODO: style
}

json_talk_repeat_response::json_talk_repeat_response( JsonObject jo )
{
    if( jo.has_bool( "is_npc" ) ) {
        is_npc = true;
    }
    if( jo.has_bool( "include_containers" ) ) {
        include_containers = true;
    }
    if( jo.has_string( "for_item" ) ) {
        for_item.emplace_back( jo.get_string( "for_item" ) );
    } else if( jo.has_array( "for_item" ) ) {
        JsonArray ja = jo.get_array( "for_item" );
        while( ja.has_more() ) {
            for_item.emplace_back( ja.next_string() );
        }
    } else if( jo.has_string( "for_category" ) ) {
        for_category.emplace_back( jo.get_string( "for_category" ) );
    } else if( jo.has_array( "for_category" ) ) {
        JsonArray ja = jo.get_array( "for_category" );
        while( ja.has_more() ) {
            for_category.emplace_back( ja.next_string() );
        }
    } else {
        jo.throw_error( "Repeat response with no repeat information!" );
    }
    if( for_item.empty() && for_category.empty() ) {
        jo.throw_error( "Repeat response with empty repeat information!" );
    }
    if( jo.has_object( "response" ) ) {
        response = json_talk_response( jo.get_object( "response" ) );
    } else {
        jo.throw_error( "Repeat response with no response!" );
    }
}

json_talk_response::json_talk_response( JsonObject jo )
    : actual_response( jo )
{
    load_condition( jo );
}

void json_talk_response::load_condition( JsonObject &jo )
{
    is_switch = jo.get_bool( "switch", false );
    is_default = jo.get_bool( "default", false );
    read_condition<dialogue>( jo, "condition", condition, true );
}

bool json_talk_response::test_condition( const dialogue &d ) const
{
    if( condition ) {
        return condition( d );
    }
    return true;
}

bool json_talk_response::gen_responses( dialogue &d, bool switch_done ) const
{
    if( !is_switch || !switch_done ) {
        if( test_condition( d ) ) {
            d.responses.emplace_back( actual_response );
            return is_switch && !is_default;
        }
    }
    return false;
}

// repeat responses always go in front
bool json_talk_response::gen_repeat_response( dialogue &d, const itype_id &item_id,
        bool switch_done ) const
{
    if( !is_switch || !switch_done ) {
        if( test_condition( d ) ) {
            talk_response result = actual_response;
            result.success.next_topic.item_type = item_id;
            result.failure.next_topic.item_type = item_id;
            d.responses.insert( d.responses.begin(), result );
            return is_switch && !is_default;
        }
    }
    return false;
}

static std::string translate_gendered_line(
    const std::string &line,
    const std::vector<std::string> &relevant_genders,
    const dialogue &d
)
{
    GenderMap gender_map;
    for( const std::string &subject : relevant_genders ) {
        if( subject == "npc" ) {
            gender_map[subject] = d.beta->get_grammatical_genders();
        } else if( subject == "u" ) {
            gender_map[subject] = d.alpha->get_grammatical_genders();
        } else {
            debugmsg( "Unsupported subject '%s' for grammatical gender in dialogue", subject );
        }
    }
    return gettext_gendered( gender_map, line );
}

dynamic_line_t dynamic_line_t::from_member( JsonObject &jo, const std::string &member_name )
{
    if( jo.has_array( member_name ) ) {
        return dynamic_line_t( jo.get_array( member_name ) );
    } else if( jo.has_object( member_name ) ) {
        return dynamic_line_t( jo.get_object( member_name ) );
    } else if( jo.has_string( member_name ) ) {
        return dynamic_line_t( jo.get_string( member_name ) );
    } else {
        return dynamic_line_t{};
    }
}

dynamic_line_t::dynamic_line_t( const std::string &line )
{
    function = [line]( const dialogue & ) {
        return _( line );
    };
}

dynamic_line_t::dynamic_line_t( JsonObject jo )
{
    if( jo.has_member( "and" ) ) {
        std::vector<dynamic_line_t> lines;
        JsonArray ja = jo.get_array( "and" );
        while( ja.has_more() ) {
            if( ja.test_string() ) {
                lines.emplace_back( ja.next_string() );
            } else if( ja.test_array() ) {
                lines.emplace_back( ja.next_array() );
            } else if( ja.test_object() ) {
                lines.emplace_back( ja.next_object() );
            } else {
                ja.throw_error( "invalid format: must be string, array or object" );
            }
        }
        function = [lines]( const dialogue & d ) {
            std::string all_lines;
            for( const dynamic_line_t &line : lines ) {
                all_lines += line( d );
            }
            return all_lines;
        };
    } else if( jo.has_member( "give_hint" ) ) {
        function = [&]( const dialogue & ) {
            return get_hint();
        };
    } else if( jo.has_member( "use_reason" ) ) {
        function = [&]( const dialogue & d ) {
            std::string tmp = d.reason;
            d.reason.clear();
            return tmp;
        };
    } else if( jo.has_string( "gendered_line" ) ) {
        const std::string line = jo.get_string( "gendered_line" );
        if( !jo.has_array( "relevant_genders" ) ) {
            jo.throw_error(
                R"(dynamic line with "gendered_line" must also have "relevant_genders")" );
        }
        JsonArray ja = jo.get_array( "relevant_genders" );
        std::vector<std::string> relevant_genders;
        while( ja.has_more() ) {
            relevant_genders.push_back( ja.next_string() );
        }
        for( const std::string &gender : relevant_genders ) {
            if( gender != "npc" && gender != "u" ) {
                jo.throw_error( "Unexpected subject in relevant_genders; expected 'npc' or 'u'" );
            }
        }
        function = [line, relevant_genders]( const dialogue & d ) {
            return translate_gendered_line( line, relevant_genders, d );
        };
    } else {
        conditional_t<dialogue> dcondition;
        const dynamic_line_t yes = from_member( jo, "yes" );
        const dynamic_line_t no = from_member( jo, "no" );
        for( const std::string &sub_member : dialogue_data::simple_string_conds ) {
            if( jo.has_bool( sub_member ) ) {
                dcondition = conditional_t<dialogue>( sub_member );
                function = [dcondition, yes, no]( const dialogue & d ) {
                    return ( dcondition( d ) ? yes : no )( d );
                };
                return;
            } else if( jo.has_member( sub_member ) ) {
                dcondition = conditional_t<dialogue>( sub_member );
                const dynamic_line_t yes_member = from_member( jo, sub_member );
                function = [dcondition, yes_member, no]( const dialogue & d ) {
                    return ( dcondition( d ) ? yes_member : no )( d );
                };
                return;
            }
        }
        for( const std::string &sub_member : dialogue_data::complex_conds ) {
            if( jo.has_member( sub_member ) ) {
                dcondition = conditional_t<dialogue>( jo );
                function = [dcondition, yes, no]( const dialogue & d ) {
                    return ( dcondition( d ) ? yes : no )( d );
                };
                return;
            }
        }
        jo.throw_error( "dynamic line not supported" );
    }
}

dynamic_line_t::dynamic_line_t( JsonArray ja )
{
    std::vector<dynamic_line_t> lines;
    while( ja.has_more() ) {
        if( ja.test_string() ) {
            lines.emplace_back( ja.next_string() );
        } else if( ja.test_array() ) {
            lines.emplace_back( ja.next_array() );
        } else if( ja.test_object() ) {
            lines.emplace_back( ja.next_object() );
        } else {
            ja.throw_error( "invalid format: must be string, array or object" );
        }
    }
    function = [lines]( const dialogue & d ) {
        const dynamic_line_t &line = random_entry_ref( lines );
        return line( d );
    };
}

json_dynamic_line_effect::json_dynamic_line_effect( JsonObject jo, const std::string &id )
{
    std::function<bool( const dialogue & )> tmp_condition;
    read_condition<dialogue>( jo, "condition", tmp_condition, true );
    talk_effect_t tmp_effect = talk_effect_t( jo );
    // if the topic has a sentinel, it means implicitly add a check for the sentinel value
    // and do not run the effects if it is set.  if it is not set, run the effects and
    // set the sentinel
    if( jo.has_string( "sentinel" ) ) {
        const std::string sentinel = jo.get_string( "sentinel" );
        const std::string varname = "npctalk_var_sentinel_" + id + "_" + sentinel;
        condition = [varname, tmp_condition]( const dialogue & d ) {
            return d.alpha->get_value( varname ) != "yes" && tmp_condition( d );
        };
        std::function<void( const dialogue &d )> function = [varname]( const dialogue & d ) {
            d.alpha->set_value( varname, "yes" );
        };
        tmp_effect.effects.emplace_back( function );
    } else {
        condition = tmp_condition;
    }
    effect = tmp_effect;
}

bool json_dynamic_line_effect::test_condition( const dialogue &d ) const
{
    return condition( d );
}

void json_dynamic_line_effect::apply( dialogue &d ) const
{
    effect.apply( d );
}

void json_talk_topic::load( JsonObject &jo )
{
    if( jo.has_member( "dynamic_line" ) ) {
        dynamic_line = dynamic_line_t::from_member( jo, "dynamic_line" );
    }
    if( jo.has_member( "speaker_effect" ) ) {
        std::string id = "no_id";
        if( jo.has_string( "id" ) ) {
            id = jo.get_string( "id" );
        } else if( jo.has_array( "id" ) ) {
            id = jo.get_array( "id" ).next_string();
        }
        if( jo.has_object( "speaker_effect" ) ) {
            speaker_effects.emplace_back( jo.get_object( "speaker_effect" ), id );
        } else if( jo.has_array( "speaker_effect" ) ) {
            JsonArray ja = jo.get_array( "speaker_effect" );
            while( ja.has_more() ) {
                speaker_effects.emplace_back( ja.next_object(), id );
            }
        }
    }
    JsonArray ja = jo.get_array( "responses" );
    responses.reserve( responses.size() + ja.size() );
    while( ja.has_more() ) {
        responses.emplace_back( ja.next_object() );
    }
    if( jo.has_object( "repeat_responses" ) ) {
        repeat_responses.emplace_back( jo.get_object( "repeat_responses" ) );
    } else if( jo.has_array( "repeat_responses" ) ) {
        ja = jo.get_array( "repeat_responses" );
        while( ja.has_more() ) {
            repeat_responses.emplace_back( ja.next_object() );
        }
    }
    if( responses.empty() ) {
        jo.throw_error( "no responses for talk topic defined", "responses" );
    }
    replace_built_in_responses = jo.get_bool( "replace_built_in_responses",
                                 replace_built_in_responses );
}

bool json_talk_topic::gen_responses( dialogue &d ) const
{
    d.responses.reserve( responses.size() ); // A wild guess, can actually be more or less

    bool switch_done = false;
    for( auto &r : responses ) {
        switch_done |= r.gen_responses( d, switch_done );
    }
    for( const json_talk_repeat_response &repeat : repeat_responses ) {
        player *actor = d.alpha;
        if( repeat.is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        std::function<bool( const item & )> filter = return_true<item>;
        for( const std::string &item_id : repeat.for_item ) {
            if( actor->charges_of( item_id ) > 0 || actor->has_amount( item_id, 1 ) ) {
                switch_done |= repeat.response.gen_repeat_response( d, item_id, switch_done );
            }
        }
        for( const std::string &category_id : repeat.for_category ) {
            const bool include_containers = repeat.include_containers;
            const auto items_with = actor->items_with( [category_id,
            include_containers]( const item & it ) {
                if( include_containers ) {
                    return it.get_category().id() == category_id;
                }
                return it.type && it.type->category && it.type->category->id() == category_id;
            } );
            for( const auto &it : items_with ) {
                switch_done |= repeat.response.gen_repeat_response( d, it->typeId(), switch_done );
            }
        }
    }

    return replace_built_in_responses;
}

std::string json_talk_topic::get_dynamic_line( const dialogue &d ) const
{
    return dynamic_line( d );
}

std::vector<json_dynamic_line_effect> json_talk_topic::get_speaker_effects() const
{
    return speaker_effects;
}

void json_talk_topic::check_consistency() const
{
    // TODO: check that all referenced topic actually exist. This is currently not possible
    // as they only exist as built in strings, not in the json_talk_topics map.
}

void unload_talk_topics()
{
    json_talk_topics.clear();
}

void load_talk_topic( JsonObject &jo )
{
    if( jo.has_array( "id" ) ) {
        for( auto &id : jo.get_string_array( "id" ) ) {
            json_talk_topics[id].load( jo );
        }
    } else {
        const std::string id = jo.get_string( "id" );
        json_talk_topics[id].load( jo );
    }
}

std::string npc::pick_talk_topic( const player &u )
{
    ( void )u;
    if( personality.aggression > 0 ) {
        if( op_of_u.fear * 2 < personality.bravery && personality.altruism < 0 ) {
            set_attitude( NPCATT_MUG );
            return "TALK_MUG";
        }

        if( personality.aggression + personality.bravery - op_of_u.fear > 0 ) {
            return "TALK_STRANGER_AGGRESSIVE";
        }
    }

    if( op_of_u.fear * 2 > personality.altruism + personality.bravery ) {
        return "TALK_STRANGER_SCARED";
    }

    if( op_of_u.fear * 2 > personality.bravery + op_of_u.trust ) {
        return "TALK_STRANGER_WARY";
    }

    if( op_of_u.trust - op_of_u.fear +
        ( personality.bravery + personality.altruism ) / 2 > 0 ) {
        return "TALK_STRANGER_FRIENDLY";
    }

    set_attitude( NPCATT_NULL );
    return "TALK_STRANGER_NEUTRAL";
}

enum consumption_result {
    REFUSED = 0,
    CONSUMED_SOME, // Consumption didn't fail, but don't delete the item
    CONSUMED_ALL   // Consumption succeeded, delete the item
};

// Returns true if we destroyed the item through consumption
static consumption_result try_consume( npc &p, item &it, std::string &reason )
{
    // TODO: Unify this with 'player::consume_item()'
    bool consuming_contents = it.is_container() && !it.contents.empty();
    item &to_eat = consuming_contents ? it.contents.front() : it;
    const auto &comest = to_eat.get_comestible();
    if( !comest ) {
        // Don't inform the player that we don't want to eat the lighter
        return REFUSED;
    }

    if( !p.will_accept_from_player( it ) ) {
        reason = _( "I don't <swear> trust you enough to eat THIS..." );
        return REFUSED;
    }

    // TODO: Make it not a copy+paste from player::consume_item
    int amount_used = 1;
    if( to_eat.is_food() ) {
        if( !p.eat( to_eat ) ) {
            reason = _( "It doesn't look like a good idea to consume this..." );
            return REFUSED;
        }
    } else if( to_eat.is_medication() || to_eat.get_contained().is_medication() ) {
        if( comest->tool != "null" ) {
            bool has = p.has_amount( comest->tool, 1 );
            if( item::count_by_charges( comest->tool ) ) {
                has = p.has_charges( comest->tool, 1 );
            }
            if( !has ) {
                reason = string_format( _( "I need a %s to consume that!" ),
                                        item::nname( comest->tool ) );
                return REFUSED;
            }
            p.use_charges( comest->tool, 1 );
        }
        if( to_eat.type->has_use() ) {
            amount_used = to_eat.type->invoke( p, to_eat, p.pos() );
            if( amount_used <= 0 ) {
                reason = _( "It doesn't look like a good idea to consume this.." );
                return REFUSED;
            }
        }

        to_eat.charges -= amount_used;
        p.consume_effects( to_eat );
        p.moves -= 250;
    } else {
        debugmsg( "Unknown comestible type of item: %s\n", to_eat.tname() );
    }

    if( to_eat.charges > 0 ) {
        return CONSUMED_SOME;
    }

    if( consuming_contents ) {
        it.contents.erase( it.contents.begin() );
        return CONSUMED_SOME;
    }

    // If not consuming contents and charge <= 0, we just ate the last charge from the stack
    return CONSUMED_ALL;
}

std::string give_item_to( npc &p, bool allow_use, bool allow_carry )
{
    if( p.is_hallucination() ) {
        return _( "No thanks, I'm good." );
    }
    const int inv_pos = g->inv_for_all( _( "Offer what?" ), _( "You have no items to offer." ) );
    item &given = g->u.i_at( inv_pos );
    if( given.is_null() ) {
        return _( "Changed your mind?" );
    }

    if( ( &given == &g->u.weapon && given.has_flag( "NO_UNWIELD" ) ) || ( g->u.is_worn( given ) &&
            given.has_flag( "NO_TAKEOFF" ) ) ) {
        // Bionic weapon or shackles
        return _( "How?" );
    }

    if( given.is_dangerous() && !g->u.has_trait( trait_DEBUG_MIND_CONTROL ) ) {
        return _( "Are you <swear> insane!?" );
    }

    std::string no_consume_reason;
    if( allow_use ) {
        // Eating first, to avoid evaluating bread as a weapon
        const auto consume_res = try_consume( p, given, no_consume_reason );
        if( consume_res == CONSUMED_ALL ) {
            g->u.i_rem( inv_pos );
        }
        if( consume_res != REFUSED ) {
            g->u.moves -= 100;
            if( given.is_container() ) {
                given.on_contents_changed();
            }
            return _( "Here we go..." );
        }
    }

    bool taken = false;
    int our_ammo = p.ammo_count_for( p.weapon );
    int new_ammo = p.ammo_count_for( given );
    const double new_weapon_value = p.weapon_value( given, new_ammo );
    const double cur_weapon_value = p.weapon_value( p.weapon, our_ammo );
    if( allow_use ) {
        add_msg( m_debug, "NPC evaluates own %s (%d ammo): %0.1f",
                 p.weapon.type->get_id(), our_ammo, cur_weapon_value );
        add_msg( m_debug, "NPC evaluates your %s (%d ammo): %0.1f",
                 given.type->get_id(), new_ammo, new_weapon_value );
        if( new_weapon_value > cur_weapon_value ) {
            p.wield( given );
            taken = true;
        }

        // is_gun here is a hack to prevent NPCs wearing guns if they don't want to use them
        if( !taken && !given.is_gun() && p.wear_if_wanted( given ) ) {
            taken = true;
        }
    }

    if( !taken && allow_carry &&
        p.can_pickVolume( given ) &&
        p.can_pickWeight( given ) ) {
        taken = true;
        p.i_add( given );
    }

    if( taken ) {
        g->u.i_rem( inv_pos );
        g->u.moves -= 100;
        p.has_new_items = true;
        return _( "Thanks!" );
    }

    std::string reason = _( "Nope." );
    if( allow_use ) {
        if( !no_consume_reason.empty() ) {
            reason += no_consume_reason + "\n";
        }

        reason += _( "My current weapon is better than this." );
        reason += "\n" + string_format( _( "(new weapon value: %.1f vs %.1f)." ), new_weapon_value,
                                        cur_weapon_value );
        if( !given.is_gun() && given.is_armor() ) {
            reason += std::string( "\n" ) + _( "It's too encumbering to wear." );
        }
    }
    if( allow_carry ) {
        if( !p.can_pickVolume( given ) ) {
            const units::volume free_space = p.volume_capacity() - p.volume_carried();
            reason += "\n" + std::string( _( "I have no space to store it." ) ) + "\n";
            if( free_space > 0_ml ) {
                reason += string_format( _( "I can only store %s %s more." ),
                                         format_volume( free_space ), volume_units_long() );
            } else {
                reason += _( "...or to store anything else for that matter." );
            }
        }
        if( !p.can_pickWeight( given ) ) {
            reason += std::string( "\n" ) + _( "It is too heavy for me to carry." );
        }
    }

    return reason;
}

bool npc::has_item_whitelist() const
{
    return is_player_ally() && !rules.pickup_whitelist->empty();
}

bool npc::item_name_whitelisted( const std::string &to_match )
{
    if( !has_item_whitelist() ) {
        return true;
    }

    auto &wlist = *rules.pickup_whitelist;
    const auto rule = wlist.check_item( to_match );
    if( rule == RULE_WHITELISTED ) {
        return true;
    }

    if( rule == RULE_BLACKLISTED ) {
        return false;
    }

    wlist.create_rule( to_match );
    return wlist.check_item( to_match ) == RULE_WHITELISTED;
}

bool npc::item_whitelisted( const item &it )
{
    if( !has_item_whitelist() ) {
        return true;
    }

    const auto to_match = it.tname( 1, false );
    return item_name_whitelisted( to_match );
}
