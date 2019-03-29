#include "dialogue.h" // IWYU pragma: associated

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#include "ammo.h"
#include "basecamp.h"
#include "cata_utility.h"
#include "catacharset.h"
// needed for the workaround for the std::to_string bug in some compilers
#include "compatibility.h" // IWYU pragma: keep
#include "coordinate_conversions.h"
#include "debug.h"
#include "editmap.h"
#include "faction_camp.h"
#include "game.h"
#include "help.h"
#include "input.h"
#include "item_group.h"
#include "itype.h"
#include "json.h"
#include "line.h"
#include "map.h"
#include "map_selector.h"
#include "martialarts.h"
#include "messages.h"
#include "mission.h"
#include "mission_companion.h"
#include "morale_types.h"
#include "npc.h"
#include "npc_class.h"
#include "npctalk.h"
#include "npctrade.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "rng.h"
#include "skill.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "text_snippets.h"
#include "translations.h"
#include "ui.h"
#include "units.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "vpart_position.h"

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

static const trait_id trait_DEBUG_MIND_CONTROL( "DEBUG_MIND_CONTROL" );

static std::map<std::string, json_talk_topic> json_talk_topics;

// Every OWED_VAL that the NPC owes you counts as +1 towards convincing
#define OWED_VAL 1000

#define dbg(x) DebugLog((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

int topic_category( const talk_topic &the_topic );

const talk_topic &special_talk( char ch );

std::string give_item_to( npc &p, bool allow_use, bool allow_carry );

std::string bulk_trade_inquire( const npc &, const itype_id &it );
void bulk_trade_accept( npc &, const itype_id &it );

const std::string &talk_trial::name() const
{
    static const std::array<std::string, NUM_TALK_TRIALS> texts = { {
            "", _( "LIE" ), _( "PERSUADE" ), _( "INTIMIDATE" ), ""
        }
    };
    if( static_cast<size_t>( type ) >= texts.size() ) {
        debugmsg( "invalid trial type %d", static_cast<int>( type ) );
        return texts[0];
    }
    return texts[type];
}

/** Time (in turns) and cost (in cent) for training: */
time_duration calc_skill_training_time( const npc &p, const skill_id &skill )
{
    return 1_minutes + 5_turns * g->u.get_skill_level( skill ) -
           1_turns * p.get_skill_level( skill );
}

int calc_skill_training_cost( const npc &p, const skill_id &skill )
{
    if( p.is_friend() ) {
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
    if( p.is_friend() ) {
        return 0;
    }

    return 800;
}

// Rescale values from "mission scale" to "opinion scale"
int cash_to_favor( const npc &, int cash )
{
    // TODO: It should affect different NPCs to a different degree
    // Square root of mission value in dollars
    // ~31 for zed mom, 50 for horde master, ~63 for plutonium cells
    double scaled_mission_val = sqrt( cash / 100.0 );
    return roll_remainder( scaled_mission_val );
}

void game::chat()
{
    const std::vector<npc *> available = get_npcs_if( [&]( const npc & guy ) {
        // TODO: Get rid of the z-level check when z-level vision gets "better"
        return u.posz() == guy.posz() &&
               u.sees( guy.pos() ) &&
               rl_dist( u.pos(), guy.pos() ) <= SEEX * 2;
    } );
    const std::vector<npc *> followers = get_npcs_if( [&]( const npc & guy ) {
        return guy.is_friend() && guy.is_following() && u.posz() == guy.posz() &&
               u.sees( guy.pos() ) && rl_dist( u.pos(), guy.pos() ) <= SEEX * 2;
    } );
    const std::vector<npc *> guards = get_npcs_if( [&]( const npc & guy ) {
        return guy.mission == NPC_MISSION_GUARD_ALLY &&
               guy.companion_mission_role_id != "FACTION_CAMP" && u.posz() == guy.posz() &&
               u.sees( guy.pos() ) && rl_dist( u.pos(), guy.pos() ) <= SEEX * 2;
    } );

    uilist nmenu;
    nmenu.text = std::string( _( "Who do you want to talk to or yell at?" ) );

    int i = 0;

    for( auto &elem : available ) {
        nmenu.addentry( i++, true, MENU_AUTOASSIGN, ( elem )->name );
    }

    int yell = 0;
    int yell_sentence = 0;
    int yell_guard = -1;
    int yell_follow = -1;
    int yell_awake = -1;
    int yell_sleep = -1;
    int yell_flee = -1;
    int yell_stop = -1;

    nmenu.addentry( yell = i++, true, 'a', _( "Yell" ) );
    nmenu.addentry( yell_sentence = i++, true, 'b', _( "Yell a sentence" ) );
    if( !followers.empty() ) {
        nmenu.addentry( yell_guard = i++, true, 'g', _( "Tell all your allies to guard" ) );
        nmenu.addentry( yell_awake = i++, true, 'w', _( "Tell all your allies to stay awake" ) );
        nmenu.addentry( yell_sleep = i++, true, 's',
                        _( "Tell all your allies to relax and sleep when tired" ) );
        nmenu.addentry( yell_flee = i++, true, 'R', _( "Tell all your allies to flee" ) );
        nmenu.addentry( yell_stop = i++, true, 'S', _( "Tell all your allies stop running" ) );
    }
    if( !guards.empty() ) {
        nmenu.addentry( yell_follow = i++, true, 'f', _( "Tell all your allies to follow" ) );
    }

    nmenu.query();
    if( nmenu.ret < 0 ) {
        return;
    } else if( nmenu.ret == yell ) {
        u.shout( _( "yourself shouting loudly!" ) );
    } else if( nmenu.ret == yell_sentence ) {
        std::string popupdesc = string_format( _( "Enter a sentence to yell" ) );
        string_input_popup popup;
        popup.title( string_format( _( "Yell a sentence" ) ) )
        .width( 64 )
        .description( popupdesc )
        .identifier( "sentence" )
        .max_length( 128 )
        .query();
        std::string sentence = popup.text();
        add_msg( _( "You yell, \"%s\"" ), sentence );
        u.shout( string_format( _( "%s yelling \"%s\"" ), u.disp_name(), sentence ) );
    } else if( nmenu.ret == yell_guard ) {
        for( npc *p : followers ) {
            talk_function::assign_guard( *p );
        }
        u.shout( _( "Guard here!" ) );
    } else if( nmenu.ret == yell_awake ) {
        for( npc *p : followers ) {
            talk_function::wake_up( *p );
        }
        u.shout( _( "Stay awake!" ) );
    } else if( nmenu.ret == yell_sleep ) {
        for( npc *p : followers ) {
            p->rules.set_flag( ally_rule::allow_sleep );
        }
        u.shout( _( "We're safe!  Take a nap if you're tired." ) );
    } else if( nmenu.ret == yell_follow ) {
        for( npc *p : guards ) {
            talk_function::stop_guard( *p );
        }
        u.shout( _( "Follow me!" ) );
    } else if( nmenu.ret == yell_flee ) {
        for( npc *p : followers ) {
            p->rules.set_flag( ally_rule::avoid_combat );
        }
        u.shout( _( "Fall back to safety!  Flee, you fools!" ) );
    } else if( nmenu.ret == yell_stop ) {
        for( npc *p : followers ) {
            p->rules.clear_flag( ally_rule::avoid_combat );
        }
        u.shout( _( "No need to run any more, we can fight here." ) );
    } else if( nmenu.ret <= static_cast<int>( available.size() ) ) {
        available[nmenu.ret]->talk_to_u();
    } else {
        return;
    }

    u.moves -= 100;
    refresh_all();
}

void npc::handle_sound( int priority, const std::string &description, int heard_volume,
                        const tripoint &spos )
{
    if( sees( spos ) ) {
        return;
    }
    // ignore low priority sounds if the NPC "knows" it came from a friend.
    // @ todo NPC will need to respond to talking noise eventually
    // but only for bantering purposes, not for investigating.
    npc *const sound_source = g->critter_at<npc>( spos );
    if( sound_source ) {
        if( ( my_fac == sound_source->my_fac ||
              get_attitude_group( get_attitude() ) == sound_source->get_attitude_group(
                  sound_source->get_attitude() ) ) && ( priority < 6 ) ) {
            add_msg( m_debug, "NPC ignored same faction %s", name.c_str() );
            return;
        }
    }
    // discount if sound source is player and listener is friendly
    if( ( priority < 6 ) && spos == g->u.pos() && ( is_friend() ||
            mission == NPC_MISSION_GUARD_ALLY ||
            get_attitude_group( get_attitude() ) != attitude_group::hostile ) ) {
        add_msg( m_debug, "NPC ignored player noise %s", name.c_str() );
        return;
    }
    // patrolling guards will investigate more readily than stationary NPCS
    int investigate_dist = 10;
    if( mission == NPC_MISSION_GUARD_ALLY || mission == NPC_MISSION_GUARD_PATROL ) {
        investigate_dist = 50;
    }
    if( priority > 3 && ai_cache.total_danger < 1.0f && rl_dist( pos(), spos ) < investigate_dist ) {
        add_msg( m_debug, "NPC %s added noise at pos %d, %d", name.c_str(), spos.x, spos.y );
        dangerous_sound temp_sound;
        temp_sound.pos = spos;
        temp_sound.volume = heard_volume;
        temp_sound.type = priority;
        if( !ai_cache.sound_alerts.empty() ) {
            if( ai_cache.sound_alerts.back().pos != spos ) {
                ai_cache.sound_alerts.push_back( temp_sound );
            }
        } else {
            ai_cache.sound_alerts.push_back( temp_sound );
        }
    }
    add_msg( m_debug, "%s heard '%s', priority %d at volume %d from %d:%d, my pos %d:%d",
             disp_name(), description, priority, heard_volume, spos.x, spos.y, pos().x, pos().y );
    switch( priority ) {
        case 7: //
            warn_about( "speech_noise", rng( 1, 10 ) * 1_minutes );
            break;
        case 6: // combat noise is only worth comment if we're not fighting
            // TODO: Brave NPCs should be less jumpy
            if( ai_cache.total_danger < 1.0f ) {
                warn_about( "combat_noise", rng( 1, 10 ) * 1_minutes );
            }
            break;
        case 4: // movement is is only worth comment if we're not fighting and out of a vehicle
            if( ai_cache.total_danger < 1.0f && !in_vehicle ) {
                warn_about( "movement_noise", rng( 1, 10 ) * 1_minutes, description );
            }
            break;
        default:
            break;
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

void npc::talk_to_u( bool text_only )
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
    } else if( !has_mind_control && get_attitude() ) {
        add_msg( _( "%s is hostile!" ), name );
        return;
    }
    dialogue d;
    d.alpha = &g->u;
    d.beta = this;

    chatbin.check_missions();

    for( auto &mission : chatbin.missions_assigned ) {
        if( mission->get_assigned_player_id() == g->u.getID() ) {
            d.missions_assigned.push_back( mission );
        }
    }

    d.add_topic( chatbin.first_topic );

    if( is_leader() ) {
        d.add_topic( "TALK_LEADER" );
    } else if( is_friend() ) {
        d.add_topic( "TALK_FRIEND" );
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
               g->u.activity.index == getID() ) {
        return;
    }
    g->cancel_activity_or_ignore_query( distraction_type::talked_to,
                                        string_format( _( "%s talked to you." ), name ) );
}

std::string dialogue::dynamic_line( const talk_topic &the_topic ) const
{
    // For compatibility
    const auto &topic = the_topic.id;
    const auto iter = json_talk_topics.find( topic );
    if( iter != json_talk_topics.end() ) {
        const std::string line = iter->second.get_dynamic_line( *this );
        if( !line.empty() ) {
            return line;
        }
    }

    if( topic == "TALK_DEAF" ) {
        return _( "&You are deaf and can't talk." );

    } else if( topic == "TALK_DEAF_ANGRY" ) {
        return string_format(
                   _( "&You are deaf and can't talk. When you don't respond, %s becomes angry!" ),
                   beta->name );
    }
    if( topic == "TALK_SEDATED" ) {
        return string_format(
                   _( "%s is sedated and can't be moved or woken up until the medication or sedation wears off." ),
                   beta->name );
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
    } else if( topic == "TALK_DELIVER_ASK" ) {
        return bulk_trade_inquire( *p, the_topic.item_type );
    } else if( topic == "TALK_DELIVER_CONFIRM" ) {
        return _( "Pleasure doing business!" );
    } else if( topic == "TALK_TRAIN" ) {
        if( !g->u.backlog.empty() && g->u.backlog.front().id() == activity_id( "ACT_TRAIN" ) ) {
            return _( "Shall we resume?" );
        }
        std::vector<skill_id> trainable = p->skills_offered_to( g->u );
        std::vector<matype_id> styles = p->styles_offered_to( g->u );
        if( trainable.empty() && styles.empty() ) {
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
    } else if( topic == "TALK_FRIEND" ) {
        return _( "What is it?" );
    } else if( topic == "TALK_DESCRIBE_MISSION" ) {
        switch( p->mission ) {
            case NPC_MISSION_SHELTER:
                return _( "I'm holing up here for safety." );
            case NPC_MISSION_SHOPKEEP:
                return _( "I run the shop here." );
            case NPC_MISSION_GUARD:
            case NPC_MISSION_GUARD_ALLY:
            case NPC_MISSION_GUARD_PATROL:
                return _( "I'm guarding this location." );
            case NPC_MISSION_NULL:
                return p->myclass.obj().get_job_description();
            default:
                return "ERROR: Someone forgot to code an npc_mission text.";
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
            return "&You can't make anything out.";
        }

        if( p->is_friend() || ability > 100 ) {
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
    } else if( topic == "TALK_USE_ITEM" ) {
        return give_item_to( *p, true, false );
    } else if( topic == "TALK_GIVE_ITEM" ) {
        return give_item_to( *p, false, true );
        // Maybe TODO: Allow an option to "just take it, use it if you want"
    } else if( topic == "TALK_MIND_CONTROL" ) {
        p->companion_mission_role_id.clear();
        p->set_attitude( NPCATT_FOLLOW );
        std::vector<int> followerlist = g->get_follower_list();
        int npc_id = p->getID();
        if( !std::any_of( followerlist.begin(), followerlist.end(), [npc_id]( int i ) {
        return i == npc_id;
    } ) ) {
            g->add_npc_follower( npc_id );
            return _( "YES, MASTER!" );
        }
    }

    return string_format( "I don't know what to say for %s. (BUG (npctalk.cpp:dynamic_line))",
                          topic );
}

talk_response &dialogue::add_response( const std::string &text, const std::string &r,
                                       const bool first )
{
    talk_response result = talk_response();
    result.truetext = text;
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
                add_response( mission->get_type().name, "TALK_MISSION_OFFER", mission, true );
            }
        }
    } else if( topic == "TALK_MISSION_LIST_ASSIGNED" ) {
        if( missions_assigned.size() == 1 ) {
            add_response( _( "I have news." ), "TALK_MISSION_INQUIRE", missions_assigned.front() );
        } else {
            for( auto &miss_it : missions_assigned ) {
                add_response( miss_it->get_type().name, "TALK_MISSION_INQUIRE", miss_it );
            }
        }
    } else if( topic == "TALK_FREE_MERCHANT_STOCKS" ) {
        add_response( _( "Who are you?" ), "TALK_FREE_MERCHANT_STOCKS_NEW", true );
        static const std::vector<itype_id> wanted = {{
                "jerky", "meat_smoked", "fish_smoked",
                "cooking_oil", "cornmeal", "flour",
                "fruit_wine", "beer", "sugar",
            }
        };

        for( const auto &id : wanted ) {
            if( g->u.charges_of( id ) > 0 ) {
                const std::string msg = string_format( _( "Delivering %s." ), item::nname( id ) );
                add_response( msg, "TALK_DELIVER_ASK", id, true );
            }
        }

    } else if( topic == "TALK_DELIVER_ASK" ) {
        if( the_topic.item_type == "null" ) {
            debugmsg( "delivering nulls" );
        }
        add_response( _( "Works for me." ), "TALK_DELIVER_CONFIRM", the_topic.item_type );
        add_response( _( "Maybe later." ), "TALK_DONE" );
    } else if( topic == "TALK_DELIVER_CONFIRM" ) {
        bulk_trade_accept( *p, the_topic.item_type );
        add_response_done( _( "You might be seeing more of me..." ) );
    } else if( topic == "TALK_RANCH_NURSE_HIRE" ) {
        if( g->u.charges_of( "bandages" ) > 0 ) {
            add_response( _( "Delivering bandages." ), "TALK_DELIVER_ASK", itype_id( "bandages" ) );
        }
    } else if( topic == "TALK_TRAIN" ) {
        if( !g->u.backlog.empty() && g->u.backlog.front().id() == activity_id( "ACT_TRAIN" ) ) {
            player_activity &backlog = g->u.backlog.front();
            std::stringstream resume;
            resume << _( "Yes, let's resume training " );
            const skill_id skillt( backlog.name );
            // TODO: This is potentially dangerous. A skill and a martial art
            // could have the same ident!
            if( !skillt.is_valid() ) {
                auto &style = matype_id( backlog.name ).obj();
                resume << style.name;
                add_response( resume.str(), "TALK_TRAIN_START", style );
            } else {
                resume << skillt.obj().name();
                add_response( resume.str(), "TALK_TRAIN_START", skillt );
            }
        }
        std::vector<matype_id> styles = p->styles_offered_to( g->u );
        std::vector<skill_id> trainable = p->skills_offered_to( g->u );
        if( trainable.empty() && styles.empty() ) {
            add_response_none( _( "Oh, okay." ) );
            return;
        }
        for( auto &style_id : styles ) {
            auto &style = style_id.obj();
            const int cost = calc_ma_style_training_cost( *p, style.id );
            //~Martial art style (cost in dollars)
            const std::string text = string_format( cost > 0 ? _( "%s ( cost $%d )" ) : "%s",
                                                    _( style.name ), cost / 100 );
            add_response( text, "TALK_TRAIN_START", style );
        }
        for( auto &trained : trainable ) {
            const int cost = calc_skill_training_cost( *p, trained );
            const int cur_level = g->u.get_skill_level( trained );
            //~Skill name: current level -> next level (cost in dollars)
            std::string text = string_format( cost > 0 ? _( "%s: %d -> %d (cost $%d)" ) :
                                              _( "%s: %d -> %d" ),
                                              trained.obj().name(), cur_level, cur_level + 1,
                                              cost / 100 );
            add_response( text, "TALK_TRAIN_START", trained );
        }
        add_response_none( _( "Eh, never mind." ) );
    } else if( topic == "TALK_HOW_MUCH_FURTHER" ) {
        add_response_none( _( "Okay, thanks." ) );
        add_response_done( _( "Let's keep moving." ) );
    }

    if( g->u.has_trait( trait_DEBUG_MIND_CONTROL ) && !p->is_friend() ) {
        add_response( _( "OBEY ME!" ), "TALK_MIND_CONTROL" );
        add_response_done( _( "Bye." ) );
    }

    if( ret.empty() ) {
        add_response_done( _( "Bye." ) );
    }
}

int parse_mod( const dialogue &d, const std::string &attribute, const int factor )
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

void parse_tags( std::string &phrase, const player &u, const player &me )
{
    phrase = remove_color_tags( phrase );

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

        const std::string &replacement = SNIPPET.random_from_category( tag );
        if( !replacement.empty() ) {
            phrase.replace( fa, l, replacement );
            continue;
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
                phrase.replace( fa, l, me.weapon.ammo_type()->name() );
            }
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
    text = truefalse_condition( d ) ? truetext : falsetext;
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
    parse_tags( ftext, *d.alpha, *d.beta );

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
    parse_tags( challenge, *alpha, *beta );
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
    std::vector<talk_data> response_lines;
    for( size_t i = 0; i < responses.size(); i++ ) {
        response_lines.push_back( responses[i].create_option_line( *this, 'a' + i ) );
    }

    long ch = text_only ? 'a' + responses.size() - 1 : ' ';
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
        } while( ( ch < 0 || ch >= static_cast<long>( responses.size() ) ) );
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
    } else if( chosen.style ) {
        beta->chatbin.style = chosen.style;
        beta->chatbin.skill = skill_id::NULL_ID();
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

    read_dialogue_condition( jo, condition, false );

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

talk_topic load_inline_topic( JsonObject jo )
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

void talk_effect_fun_t::set_companion_mission( const std::string &role_id )
{
    function = [role_id]( const dialogue & d ) {
        npc &p = *d.beta;
        p.companion_mission_role_id = role_id;
        talk_function::companion_mission( p );
    };
}

void talk_effect_fun_t::set_add_effect( bool is_u, const std::string &new_effect,
                                        const time_duration &duration, bool permanent )
{
    function = [is_u, new_effect, duration, permanent]( const dialogue & d ) {
        if( is_u ) {
            player &u = *d.alpha;
            u.add_effect( efftype_id( new_effect ), duration, num_bp, permanent );
        } else {
            npc &p = *d.beta;
            p.add_effect( efftype_id( new_effect ), duration, num_bp, permanent );
        }
    };
}

void talk_effect_fun_t::set_remove_effect( bool is_u, const std::string &old_effect )
{
    function = [is_u, old_effect]( const dialogue & d ) {
        if( is_u ) {
            player &u = *d.alpha;
            u.remove_effect( efftype_id( old_effect ), num_bp );
        } else {
            npc &p = *d.beta;
            p.remove_effect( efftype_id( old_effect ), num_bp );
        }
    };
}

void talk_effect_fun_t::set_add_trait( bool is_u, const std::string &new_trait )
{
    function = [is_u, new_trait]( const dialogue & d ) {
        if( is_u ) {
            player &u = *d.alpha;
            u.set_mutation( trait_id( new_trait ) );
        } else {
            npc &p = *d.beta;
            p.set_mutation( trait_id( new_trait ) );
        }
    };
}

void talk_effect_fun_t::set_remove_trait( bool is_u, const std::string &old_trait )
{
    function = [is_u, old_trait]( const dialogue & d ) {
        if( is_u ) {
            player &u = *d.alpha;
            u.unset_mutation( trait_id( old_trait ) );
        } else {
            npc &p = *d.beta;
            p.unset_mutation( trait_id( old_trait ) );
        }
    };
}

void talk_effect_fun_t::set_u_buy_item( const std::string &item_name, int cost, int count,
                                        const std::string &container_name )
{
    function = [item_name, cost, count, container_name]( const dialogue & d ) {
        npc &p = *d.beta;
        player &u = *d.alpha;
        if( container_name.empty() ) {
            item new_item = item( item_name, calendar::turn, count );
            u.i_add( new_item );
            if( count == 1 ) {
                //~ %1%s is the NPC name, %2$s is an item
                popup( _( "%1$s gives you a %2$s" ), p.name, new_item.tname() );
            } else {
                //~ %1%s is the NPC name, %2$d is a number of items, %3$s are items
                popup( _( "%1$s gives you %2$d %3$s" ), p.name, count, new_item.tname() );
            }
        } else {
            item container( container_name, calendar::turn );
            container.emplace_back( item_name, calendar::turn, count );
            u.i_add( container );
            //~ %1%s is the NPC name, %2$s is an item
            popup( _( "%1$s gives you a %2$s" ), p.name, container.tname() );
        }
        u.cash -= cost;
    };
}

void talk_effect_fun_t::set_u_sell_item( const std::string &item_name, int cost, int count )
{
    function = [item_name, cost, count]( const dialogue & d ) {
        npc &p = *d.beta;
        player &u = *d.alpha;
        item old_item = item( item_name, calendar::turn, count );
        if( u.has_charges( item_name, count ) ) {
            u.use_charges( item_name, count );
        } else if( u.has_amount( item_name, count ) ) {
            u.use_amount( item_name, count );
        } else {
            //~ %1$s is a translated item name
            popup( _( "You don't have a %1$s!" ), old_item.tname() );
            return;
        }
        p.i_add( old_item );

        if( count == 1 ) {
            //~ %1%s is the NPC name, %2$s is an item
            popup( _( "You give %1$s a %2$s" ), p.name, old_item.tname() );
        } else {
            //~ %1%s is the NPC name, %2$d is a number of items, %3$s are items
            popup( _( "You give %1$s %2$d %3$s" ), p.name, count, old_item.tname() );
        }
        u.cash += cost;
    };
}

void talk_effect_fun_t::set_consume_item( bool is_u, const std::string &item_name, int count )
{
    function = [is_u, item_name, count]( const dialogue & d ) {
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
        if( is_u ) {
            consume_item( *d.alpha, item_name, count );
        } else {
            consume_item( *d.beta, item_name, count );
        }
    };
}

void talk_effect_fun_t::set_u_spend_cash( int amount )
{
    function = [amount]( const dialogue & d ) {
        player &u = *d.alpha;
        u.cash -= amount;
    };
}

void talk_effect_fun_t::set_npc_change_faction( const std::string &faction_name )
{
    function = [faction_name]( const dialogue & d ) {
        npc &p = *d.beta;
        p.my_fac = g->faction_manager_ptr->get( faction_id( faction_name ) );
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
        p.my_fac->likes_u += rep_change;
        p.my_fac->respects_u += rep_change;
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
        d.beta->rules.toggle_flag( toggle->second );
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
        int m_value = cash_to_favor( *d.beta, miss->get_value() );
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
    } else {
        next_topic = talk_topic( jo.get_string( "topic" ) );
    }
}

void talk_effect_t::parse_sub_effect( JsonObject jo )
{
    talk_effect_fun_t subeffect_fun;
    if( jo.has_string( "companion_mission" ) ) {
        std::string role_id = jo.get_string( "companion_mission" );
        subeffect_fun.set_companion_mission( role_id );
    } else if( jo.has_string( "u_add_effect" )  || jo.has_string( "npc_add_effect" ) ) {
        std::string new_effect;
        bool is_u = true;
        if( jo.has_string( "u_add_effect" ) ) {
            new_effect  = jo.get_string( "u_add_effect" );
        } else {
            new_effect  = jo.get_string( "npc_add_effect" );
            is_u = false;
        }
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
        subeffect_fun.set_add_effect( is_u, new_effect, duration, permanent );
    } else if( jo.has_string( "u_lose_effect" ) ) {
        std::string old_effect = jo.get_string( "u_lose_effect" );
        subeffect_fun.set_remove_effect( true, old_effect );
    } else if( jo.has_string( "npc_lose_effect" ) ) {
        std::string old_effect = jo.get_string( "npc_lose_effect" );
        subeffect_fun.set_remove_effect( false, old_effect );
    } else if( jo.has_string( "u_add_trait" ) ) {
        std::string new_trait = jo.get_string( "u_add_trait" );
        subeffect_fun.set_add_trait( true, new_trait );
    } else if( jo.has_string( "npc_add_trait" ) ) {
        std::string new_trait = jo.get_string( "npc_add_trait" );
        subeffect_fun.set_add_trait( false, new_trait );
    } else if( jo.has_string( "u_lose_trait" ) ) {
        std::string old_trait = jo.get_string( "u_lose_trait" );
        subeffect_fun.set_remove_trait( true, old_trait );
    } else if( jo.has_string( "npc_lose_trait" ) ) {
        std::string old_trait = jo.get_string( "npc_lose_trait" );
        subeffect_fun.set_remove_trait( false, old_trait );
    } else if( jo.has_int( "u_spend_cash" ) ) {
        int cash_change = jo.get_int( "u_spend_cash" );
        subeffect_fun.set_u_spend_cash( cash_change );
    } else if( jo.has_string( "u_sell_item" ) || jo.has_string( "u_buy_item" ) ||
               jo.has_string( "u_consume_item" ) || jo.has_string( "npc_consume_item" ) ) {
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
        std::string item_name;
        if( jo.has_string( "u_sell_item" ) ) {
            item_name = jo.get_string( "u_sell_item" );
            subeffect_fun.set_u_sell_item( item_name, cost, count );
        } else if( jo.has_string( "u_buy_item" ) ) {
            item_name = jo.get_string( "u_buy_item" );
            subeffect_fun.set_u_buy_item( item_name, cost, count, container_name );
        } else if( jo.has_string( "u_consume_item" ) || jo.has_string( "npc_consume_item" ) ) {
            bool is_u = true;
            if( jo.has_string( "u_consume_item" ) ) {
                item_name = jo.get_string( "u_consume_item" );
            } else {
                item_name = jo.get_string( "npc_consume_item" );
                is_u = false;
            }
            subeffect_fun.set_consume_item( is_u, item_name, count );
        }
    } else if( jo.has_string( "npc_change_class" ) ) {
        std::string class_name = jo.get_string( "npc_change_class" );
        subeffect_fun.set_npc_change_class( class_name );
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
    } else if( jo.has_string( "set_npc_engagement_rule" ) ) {
        const std::string setting = jo.get_string( "set_npc_engagement_rule" );
        subeffect_fun.set_npc_engagement_rule( setting );
    } else if( jo.has_string( "set_npc_aim_rule" ) ) {
        const std::string setting = jo.get_string( "set_npc_aim_rule" );
        subeffect_fun.set_npc_aim_rule( setting );
    } else {
        jo.throw_error( "invalid sub effect syntax :" + jo.str() );
    }
    set_effect( subeffect_fun );
}

void talk_effect_t::parse_string_effect( const std::string &type, JsonObject &jo )
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
            WRAP( assign_guard ),
            WRAP( stop_guard ),
            WRAP( start_camp ),
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
            WRAP( buy_haircut ),
            WRAP( buy_shave ),
            WRAP( morale_chat ),
            WRAP( morale_chat_activity ),
            WRAP( buy_10_logs ),
            WRAP( buy_100_logs ),
            WRAP( bionic_install ),
            WRAP( bionic_remove ),
            WRAP( follow ),
            WRAP( deny_follow ),
            WRAP( deny_lead ),
            WRAP( deny_equipment ),
            WRAP( deny_train ),
            WRAP( deny_personal_info ),
            WRAP( hostile ),
            WRAP( flee ),
            WRAP( leave ),
            WRAP( stranger_neutral ),
            WRAP( start_mugging ),
            WRAP( player_leaving ),
            WRAP( drop_weapon ),
            WRAP( player_weapon_away ),
            WRAP( player_weapon_drop ),
            WRAP( lead_to_safety ),
            WRAP( start_training ),
            WRAP( copy_npc_rules ),
            WRAP( set_npc_pickup ),
            WRAP( nothing )
#undef WRAP
        }
    };
    const auto iter = static_functions_map.find( type );
    if( iter != static_functions_map.end() ) {
        set_effect( iter->second );
        return;
    }
    // more functions can be added here, they don't need to be in the map above.
    jo.throw_error( "unknown effect string", type );
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
}

talk_response::talk_response( JsonObject jo )
{
    if( jo.has_member( "truefalsetext" ) ) {
        JsonObject truefalse_jo = jo.get_object( "truefalsetext" );
        read_dialogue_condition( truefalse_jo, truefalse_condition, true );
        truetext = _( truefalse_jo.get_string( "true" ) );
        falsetext = _( truefalse_jo.get_string( "false" ) );
    } else {
        truetext = _( jo.get_string( "text" ) );
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

json_talk_response::json_talk_response( JsonObject jo )
    : actual_response( jo )
{
    load_condition( jo );
}

void read_dialogue_condition( JsonObject &jo, std::function<bool( const dialogue & )> &condition,
                              bool default_val )
{
    const auto null_function = [default_val]( const dialogue & ) {
        return default_val;
    };

    static const std::string member_name( "condition" );
    if( !jo.has_member( member_name ) ) {
        condition = null_function;
    } else if( jo.has_string( member_name ) ) {
        const std::string type = jo.get_string( member_name );
        conditional_t sub_condition( type );
        condition = [sub_condition]( const dialogue & d ) {
            return sub_condition( d );
        };
    } else if( jo.has_object( member_name ) ) {
        const JsonObject con_obj = jo.get_object( member_name );
        conditional_t sub_condition( con_obj );
        condition = [sub_condition]( const dialogue & d ) {
            return sub_condition( d );
        };
    } else {
        jo.throw_error( "invalid condition syntax", member_name );
    }
}

void conditional_t::set_has_any_trait( JsonObject &jo, const std::string &member, bool is_npc )
{
    std::vector<trait_id> traits_to_check;
    for( auto &&f : jo.get_string_array( member ) ) { // *NOPAD*
        traits_to_check.emplace_back( f );
    }
    condition = [traits_to_check, is_npc]( const dialogue & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        for( const auto &trait : traits_to_check ) {
            if( actor->has_trait( trait ) ) {
                return true;
            }
        }
        return false;
    };
}

void conditional_t::set_has_trait( JsonObject &jo, const std::string &member, bool is_npc )
{
    const std::string &trait_to_check = jo.get_string( member );
    condition = [trait_to_check, is_npc]( const dialogue & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        return actor->has_trait( trait_id( trait_to_check ) );
    };
}

void conditional_t::set_has_trait_flag( JsonObject &jo, const std::string &member, bool is_npc )
{
    const std::string &trait_flag_to_check = jo.get_string( member );
    condition = [trait_flag_to_check, is_npc]( const dialogue & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        if( trait_flag_to_check == "MUTATION_THRESHOLD" ) {
            return actor->crossed_threshold();
        }
        return actor->has_trait_flag( trait_flag_to_check );
    };
}

void conditional_t::set_npc_has_class( JsonObject &jo )
{
    const std::string &class_to_check = jo.get_string( "npc_has_class" );
    condition = [class_to_check]( const dialogue & d ) {
        return d.beta->myclass == npc_class_id( class_to_check );
    };
}

void conditional_t::set_u_has_mission( JsonObject &jo )
{
    const std::string &mission = jo.get_string( "u_has_mission" );
    condition = [mission]( const dialogue & ) {
        for( auto miss_it : g->u.get_active_missions() ) {
            if( miss_it->mission_id() == mission_type_id( mission ) ) {
                return true;
            }
        }
        return false;
    };
}

void conditional_t::set_has_strength( JsonObject &jo, const std::string &member, bool is_npc )
{
    const int min_strength = jo.get_int( member );
    condition = [min_strength, is_npc]( const dialogue & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        return actor->str_cur >= min_strength;
    };
}

void conditional_t::set_has_dexterity( JsonObject &jo, const std::string &member, bool is_npc )
{
    const int min_dexterity = jo.get_int( member );
    condition = [min_dexterity, is_npc]( const dialogue & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        return actor->dex_cur >= min_dexterity;
    };
}

void conditional_t::set_has_intelligence( JsonObject &jo, const std::string &member, bool is_npc )
{
    const int min_intelligence = jo.get_int( member );
    condition = [min_intelligence, is_npc]( const dialogue & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        return actor->int_cur >= min_intelligence;
    };
}

void conditional_t::set_has_perception( JsonObject &jo, const std::string &member, bool is_npc )
{
    const int min_perception = jo.get_int( member );
    condition = [min_perception, is_npc]( const dialogue & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        return actor->per_cur >= min_perception;
    };
}

void conditional_t::set_is_wearing( JsonObject &jo, const std::string &member, bool is_npc )
{
    const std::string &item_id = jo.get_string( member );
    condition = [item_id, is_npc]( const dialogue & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        return actor->is_wearing( item_id );
    };
}

void conditional_t::set_has_item( JsonObject &jo, const std::string &member, bool is_npc )
{
    const std::string &item_id = jo.get_string( member );
    condition = [item_id, is_npc]( const dialogue & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        return actor->charges_of( item_id ) > 0 || actor->has_amount( item_id, 1 );
    };
}

void conditional_t::set_has_items( JsonObject &jo, const std::string &member, bool is_npc )
{
    JsonObject has_items = jo.get_object( member );
    if( !has_items.has_string( "item" ) || !has_items.has_int( "count" ) ) {
        condition = []( const dialogue & ) {
            return false;
        };
    } else {
        const std::string item_id = has_items.get_string( "item" );
        int count = has_items.get_int( "count" );
        condition = [item_id, count, is_npc]( const dialogue & d ) {
            player *actor = d.alpha;
            if( is_npc ) {
                actor = dynamic_cast<player *>( d.beta );
            }
            return actor->has_charges( item_id, count ) || actor->has_amount( item_id, count );
        };
    }
}

void conditional_t::set_has_effect( JsonObject &jo, const std::string &member, bool is_npc )
{
    const std::string &effect_id = jo.get_string( member );
    condition = [effect_id, is_npc]( const dialogue & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        return actor->has_effect( efftype_id( effect_id ) );
    };
}

void conditional_t::set_need( JsonObject &jo, const std::string &member, bool is_npc )
{
    const std::string &need = jo.get_string( member );
    int amount = 0;
    if( jo.has_int( "amount" ) ) {
        amount = jo.get_int( "amount" );
    } else if( jo.has_string( "level" ) ) {
        const std::string &level = jo.get_string( "level" );
        auto flevel = fatigue_level_strs.find( level );
        if( flevel != fatigue_level_strs.end() ) {
            amount = static_cast<int>( flevel->second );
        }
    }
    condition = [need, amount, is_npc]( const dialogue & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        return ( actor->get_fatigue() > amount && need == "fatigue" ) ||
               ( actor->get_hunger() > amount && need == "hunger" ) ||
               ( actor->get_thirst() > amount && need == "thirst" );
    };
}

void conditional_t::set_at_om_location( JsonObject &jo, const std::string &member, bool is_npc )
{
    const std::string &location = jo.get_string( member );
    condition = [location, is_npc]( const dialogue & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        const tripoint omt_pos = actor->global_omt_location();
        oter_id &omt_ref = overmap_buffer.ter( omt_pos );

        if( location == "FACTION_CAMP_ANY" ) {
            cata::optional<basecamp *> bcp = overmap_buffer.find_camp( omt_pos.x, omt_pos.y );
            if( bcp ) {
                return true;
            }
            // legacy check
            const std::string &omt_str = omt_ref.id().c_str();
            return omt_str.find( "faction_base_camp" ) != std::string::npos;
        } else {
            return omt_ref == oter_id( location );
        }
    };
}

void conditional_t::set_npc_role_nearby( JsonObject &jo )
{
    const std::string &role = jo.get_string( "npc_role_nearby" );
    condition = [role]( const dialogue & d ) {
        const std::vector<npc *> available = g->get_npcs_if( [&]( const npc & guy ) {
            return d.alpha->posz() == guy.posz() && guy.companion_mission_role_id == role &&
                   ( rl_dist( d.alpha->pos(), guy.pos() ) <= 48 );
        } );
        return !available.empty();
    };
}

void conditional_t::set_npc_allies( JsonObject &jo )
{
    const unsigned long min_allies = jo.get_int( "npc_allies" );
    condition = [min_allies]( const dialogue & ) {
        return g->allies().size() >= min_allies;
    };
}

void conditional_t::set_npc_service( JsonObject &jo )
{
    const unsigned long service_price = jo.get_int( "npc_service" );
    condition = [service_price]( const dialogue & d ) {
        return !d.beta->has_effect( effect_currently_busy ) && d.alpha->cash >= service_price;
    };
}

void conditional_t::set_u_has_cash( JsonObject &jo )
{
    const unsigned long min_cash = jo.get_int( "u_has_cash" );
    condition = [min_cash]( const dialogue & d ) {
        return d.alpha->cash >= min_cash;
    };
}

void conditional_t::set_npc_aim_rule( JsonObject &jo )
{
    const std::string &setting = jo.get_string( "npc_aim_rule" );
    condition = [setting]( const dialogue & d ) {
        auto rule = aim_rule_strs.find( setting );
        if( rule != aim_rule_strs.end() ) {
            return d.beta->rules.aim == rule->second;
        }
        return false;
    };
}

void conditional_t::set_npc_engagement_rule( JsonObject &jo )
{
    const std::string &setting = jo.get_string( "npc_engagement_rule" );
    condition = [setting]( const dialogue & d ) {
        auto rule = combat_engagement_strs.find( setting );
        if( rule != combat_engagement_strs.end() ) {
            return d.beta->rules.engagement == rule->second;
        }
        return false;
    };
}

void conditional_t::set_npc_rule( JsonObject &jo )
{
    std::string rule = jo.get_string( "npc_rule" );
    condition = [rule]( const dialogue & d ) {
        auto flag = ally_rule_strs.find( rule );
        if( flag != ally_rule_strs.end() ) {
            return d.beta->rules.has_flag( flag->second );
        }
        return false;
    };
}

void conditional_t::set_days_since( JsonObject &jo )
{
    const unsigned long days = jo.get_int( "days_since_cataclysm" );
    condition = [days]( const dialogue & ) {
        return to_turn<int>( calendar::turn ) >= DAYS( days );
    };
}

void conditional_t::set_is_season( JsonObject &jo )
{
    std::string season_name = jo.get_string( "is_season" );
    condition = [season_name]( const dialogue & ) {
        const auto season = season_of_year( calendar::turn );
        return ( season == SPRING && season_name == "spring" ) ||
               ( season == SUMMER && season_name == "summer" ) ||
               ( season == AUTUMN && season_name == "autumn" ) ||
               ( season == WINTER && season_name == "winter" );
    };
}

void conditional_t::set_mission_goal( JsonObject &jo )
{
    std::string mission_goal_str = jo.get_string( "mission_goal" );
    condition = [mission_goal_str]( const dialogue & d ) {
        mission *miss = d.beta->chatbin.mission_selected;
        const auto mgoal = mission_goal_strs.find( mission_goal_str );
        if( !miss || mgoal == mission_goal_strs.end() ) {
            return false;
        }
        return miss->get_type().goal == mgoal->second;
    };
}

void conditional_t::set_is_gender( bool is_male, bool is_npc )
{
    condition = [is_male, is_npc]( const dialogue & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        return actor->male == is_male;
    };
}

void conditional_t::set_no_assigned_mission()
{
    condition = []( const dialogue & d ) {
        return d.missions_assigned.empty();
    };
}

void conditional_t::set_has_assigned_mission()
{
    condition = []( const dialogue & d ) {
        return d.missions_assigned.size() == 1;
    };
}

void conditional_t::set_has_many_assigned_missions()
{
    condition = []( const dialogue & d ) {
        return d.missions_assigned.size() >= 2;
    };
}

void conditional_t::set_no_available_mission()
{
    condition = []( const dialogue & d ) {
        return d.beta->chatbin.missions.empty();
    };
}

void conditional_t::set_has_available_mission()
{
    condition = []( const dialogue & d ) {
        return d.beta->chatbin.missions.size() == 1;
    };
}

void conditional_t::set_has_many_available_missions()
{
    condition = []( const dialogue & d ) {
        return d.beta->chatbin.missions.size() >= 2;
    };
}

void conditional_t::set_mission_complete()
{
    condition = []( const dialogue & d ) {
        mission *miss = d.beta->chatbin.mission_selected;
        if( !miss ) {
            return false;
        }
        return miss->is_complete( d.beta->getID() );
    };
}

void conditional_t::set_mission_incomplete()
{
    condition = []( const dialogue & d ) {
        mission *miss = d.beta->chatbin.mission_selected;
        if( !miss ) {
            return false;
        }
        return !miss->is_complete( d.beta->getID() );
    };
}

void conditional_t::set_npc_available()
{
    condition = []( const dialogue & d ) {
        return !d.beta->has_effect( effect_currently_busy );
    };
}

void conditional_t::set_npc_following()
{
    condition = []( const dialogue & d ) {
        return d.beta->is_following();
    };
}

void conditional_t::set_npc_friend()
{
    condition = []( const dialogue & d ) {
        return d.beta->is_friend();
    };
}

void conditional_t::set_npc_hostile()
{
    condition = []( const dialogue & d ) {
        return d.beta->is_enemy();
    };
}

void conditional_t::set_npc_train_skills()
{
    condition = []( const dialogue & d ) {
        return !d.beta->skills_offered_to( *d.alpha ).empty();
    };
}

void conditional_t::set_npc_train_styles()
{
    condition = []( const dialogue & d ) {
        return !d.beta->styles_offered_to( *d.alpha ).empty();
    };
}

void conditional_t::set_at_safe_space()
{
    condition = []( const dialogue & d ) {
        return overmap_buffer.is_safe( d.beta->global_omt_location() );
    };
}

void conditional_t::set_can_stow_weapon( bool is_npc )
{
    condition = [is_npc]( const dialogue & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        return !actor->unarmed_attack() && actor->can_pickVolume( actor->weapon );
    };
}

void conditional_t::set_has_weapon( bool is_npc )
{
    condition = [is_npc]( const dialogue & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        return !actor->unarmed_attack();
    };
}

void conditional_t::set_is_driving( bool is_npc )
{
    condition = [is_npc]( const dialogue & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        if( const optional_vpart_position vp = g->m.veh_at( actor->pos() ) ) {
            return vp->vehicle().is_moving() && vp->vehicle().player_in_control( *actor );
        }
        return false;
    };
}

void conditional_t::set_is_day()
{
    condition = []( const dialogue & ) {
        return !calendar::turn.is_night();
    };
}

void conditional_t::set_is_outside()
{
    condition = []( const dialogue & d ) {
        const tripoint pos = g->m.getabs( d.beta->pos() );
        return !g->m.has_flag( TFLAG_INDOORS, pos );
    };
}

void conditional_t::set_u_has_camp()
{
    condition = []( const dialogue & ) {
        return !g->u.camps.empty();
    };
}

void conditional_t::set_has_pickup_list()
{
    condition = []( const dialogue & d ) {
        return !d.beta->rules.pickup_whitelist->empty();
    };
}

conditional_t::conditional_t( JsonObject jo )
{
    // improve the clarity of NPC setter functions
    const bool is_npc = true;
    const auto parse_array = []( JsonObject jo, const std::string & type ) {
        std::vector<conditional_t> conditionals;
        JsonArray ja = jo.get_array( type );
        while( ja.has_more() ) {
            if( ja.test_string() ) {
                conditional_t type_condition( ja.next_string() );
                conditionals.emplace_back( type_condition );
            } else if( ja.test_object() ) {
                conditional_t type_condition( ja.next_object() );
                conditionals.emplace_back( type_condition );
            } else {
                ja.skip_value();
            }
        }
        return conditionals;
    };
    if( jo.has_array( "and" ) ) {
        std::vector<conditional_t> and_conditionals = parse_array( jo, "and" );
        condition = [and_conditionals]( const dialogue & d ) {
            for( const auto &cond : and_conditionals ) {
                if( !cond( d ) ) {
                    return false;
                }
            }
            return true;
        };
    } else if( jo.has_array( "or" ) ) {
        std::vector<conditional_t> or_conditionals = parse_array( jo, "or" );
        condition = [or_conditionals]( const dialogue & d ) {
            for( const auto &cond : or_conditionals ) {
                if( cond( d ) ) {
                    return true;
                }
            }
            return false;
        };
    } else if( jo.has_object( "not" ) ) {
        const conditional_t sub_condition = conditional_t( jo.get_object( "not" ) );
        condition = [sub_condition]( const dialogue & d ) {
            return !sub_condition( d );
        };
    } else if( jo.has_string( "not" ) ) {
        const conditional_t sub_condition = conditional_t( jo.get_string( "not" ) );
        condition = [sub_condition]( const dialogue & d ) {
            return !sub_condition( d );
        };
    } else if( jo.has_member( "u_has_any_trait" ) ) {
        set_has_any_trait( jo, "u_has_any_trait" );
    } else if( jo.has_member( "npc_has_any_trait" ) ) {
        set_has_any_trait( jo, "npc_has_any_trait", true );
    } else if( jo.has_member( "u_has_trait" ) ) {
        set_has_trait( jo, "u_has_trait" );
    } else if( jo.has_member( "npc_has_trait" ) ) {
        set_has_trait( jo, "npc_has_trait", true );
    } else if( jo.has_member( "u_has_trait_flag" ) ) {
        set_has_trait_flag( jo, "u_has_trait_flag" );
    } else if( jo.has_member( "npc_has_trait_flag" ) ) {
        set_has_trait_flag( jo, "npc_has_trait_flag", true );
    } else if( jo.has_member( "npc_has_class" ) ) {
        set_npc_has_class( jo );
    } else if( jo.has_string( "u_has_mission" ) ) {
        set_u_has_mission( jo );
    } else if( jo.has_int( "u_has_strength" ) ) {
        set_has_strength( jo, "u_has_strength" );
    } else if( jo.has_int( "npc_has_strength" ) ) {
        set_has_strength( jo, "npc_has_strength", is_npc );
    } else if( jo.has_int( "u_has_dexterity" ) ) {
        set_has_dexterity( jo, "u_has_dexterity" );
    } else if( jo.has_int( "npc_has_dexterity" ) ) {
        set_has_dexterity( jo, "npc_has_dexterity", is_npc );
    } else if( jo.has_int( "u_has_intelligence" ) ) {
        set_has_intelligence( jo, "u_has_intelligence" );
    } else if( jo.has_int( "npc_has_intelligence" ) ) {
        set_has_intelligence( jo, "npc_has_intelligence", is_npc );
    } else if( jo.has_int( "u_has_perception" ) ) {
        set_has_perception( jo, "u_has_perception" );
    } else if( jo.has_int( "npc_has_perception" ) ) {
        set_has_perception( jo, "npc_has_perception", is_npc );
    } else if( jo.has_string( "u_is_wearing" ) ) {
        set_is_wearing( jo, "u_is_wearing" );
    } else if( jo.has_string( "npc_is_wearing" ) ) {
        set_is_wearing( jo, "npc_is_wearing", is_npc );
    } else if( jo.has_string( "u_has_item" ) ) {
        set_has_item( jo, "u_has_item" );
    } else if( jo.has_string( "npc_has_item" ) ) {
        set_has_item( jo, "npc_has_item", is_npc );
    } else if( jo.has_member( "u_has_items" ) ) {
        set_has_items( jo, "u_has_items" );
    } else if( jo.has_member( "npc_has_items" ) ) {
        set_has_items( jo, "npc_has_items", is_npc );
    } else if( jo.has_string( "u_has_effect" ) ) {
        set_has_effect( jo, "u_has_effect" );
    } else if( jo.has_string( "npc_has_effect" ) ) {
        set_has_effect( jo, "npc_has_effect", is_npc );
    } else if( jo.has_string( "u_need" ) ) {
        set_need( jo, "u_need" );
    } else if( jo.has_string( "npc_need" ) ) {
        set_need( jo, "npc_need", is_npc );
    } else if( jo.has_string( "u_at_om_location" ) ) {
        set_at_om_location( jo, "u_at_om_location" );
    } else if( jo.has_string( "npc_at_om_location" ) ) {
        set_at_om_location( jo, "npc_at_om_location", is_npc );
    } else if( jo.has_string( "npc_role_nearby" ) ) {
        set_npc_role_nearby( jo );
    } else if( jo.has_int( "npc_allies" ) ) {
        set_npc_allies( jo );
    } else if( jo.has_int( "npc_service" ) ) {
        set_npc_service( jo );
    } else if( jo.has_int( "u_has_cash" ) ) {
        set_u_has_cash( jo );
    } else if( jo.has_string( "npc_aim_rule" ) ) {
        set_npc_aim_rule( jo );
    } else if( jo.has_string( "npc_engagement_rule" ) ) {
        set_npc_engagement_rule( jo );
    } else if( jo.has_string( "npc_rule" ) ) {
        set_npc_rule( jo );
    } else if( jo.has_int( "days_since_cataclysm" ) ) {
        set_days_since( jo );
    } else if( jo.has_string( "is_season" ) ) {
        set_is_season( jo );
    } else if( jo.has_string( "mission_goal" ) ) {
        set_mission_goal( jo );
    } else {
        bool found_sub_member = false;
        for( const std::string &sub_member : dialogue_data::simple_string_conds ) {
            if( jo.has_string( sub_member ) ) {
                const conditional_t sub_condition( jo.get_string( sub_member ) );
                condition = [sub_condition]( const dialogue & d ) {
                    return sub_condition( d );
                };
                found_sub_member = true;
                break;
            }
        }
        if( !found_sub_member ) {
            condition = []( const dialogue & ) {
                return false;
            };
        }
    }
}

conditional_t::conditional_t( const std::string &type )
{
    const bool is_npc = true;
    if( type == "u_male" ) {
        set_is_gender( true );
    } else if( type == "npc_male" ) {
        set_is_gender( true, is_npc );
    } else if( type == "u_female" ) {
        set_is_gender( false );
    } else if( type == "npc_female" ) {
        set_is_gender( false, is_npc );
    } else if( type == "has_no_assigned_mission" ) {
        set_no_assigned_mission();
    } else if( type == "has_assigned_mission" ) {
        set_has_assigned_mission();
    } else if( type == "has_many_assigned_missions" ) {
        set_has_many_assigned_missions();
    } else if( type == "has_no_available_mission" ) {
        set_no_available_mission();
    } else if( type == "has_available_mission" ) {
        set_has_available_mission();
    } else if( type == "has_many_available_missions" ) {
        set_has_many_available_missions();
    } else if( type == "mission_complete" ) {
        set_mission_complete();
    } else if( type == "mission_incomplete" ) {
        set_mission_incomplete();
    } else if( type == "npc_available" ) {
        set_npc_available();
    } else if( type == "npc_following" ) {
        set_npc_following();
    } else if( type == "npc_friend" ) {
        set_npc_friend();
    } else if( type == "npc_hostile" ) {
        set_npc_hostile();
    } else if( type == "npc_train_skills" ) {
        set_npc_train_skills();
    } else if( type == "npc_train_styles" ) {
        set_npc_train_styles();
    } else if( type == "at_safe_space" ) {
        set_at_safe_space();
    } else if( type == "u_can_stow_weapon" ) {
        set_can_stow_weapon();
    } else if( type == "npc_can_stow_weapon" ) {
        set_can_stow_weapon( is_npc );
    } else if( type == "u_has_weapon" ) {
        set_has_weapon();
    } else if( type == "npc_has_weapon" ) {
        set_has_weapon( is_npc );
    } else if( type == "u_driving" ) {
        set_is_driving();
    } else if( type == "npc_driving" ) {
        set_is_driving( is_npc );
    } else if( type == "is_day" ) {
        set_is_day();
    } else if( type == "is_outside" ) {
        set_is_outside();
    } else if( type == "u_has_camp" ) {
        set_u_has_camp();
    } else if( type == "has_pickup_list" ) {
        set_has_pickup_list();
    } else {
        condition = []( const dialogue & ) {
            return false;
        };
    }
}

void json_talk_response::load_condition( JsonObject &jo )
{
    is_switch = jo.get_bool( "switch", false );
    is_default = jo.get_bool( "default", false );
    read_dialogue_condition( jo, condition, true );
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
    if( test_condition( d ) ) {
        if( !is_switch || ( is_switch && !switch_done ) ) {
            d.responses.emplace_back( actual_response );
        }
        return is_switch && !is_default;
    }
    return false;
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
    } else {
        conditional_t dcondition;
        const dynamic_line_t yes = from_member( jo, "yes" );
        const dynamic_line_t no = from_member( jo, "no" );
        for( const std::string &sub_member : dialogue_data::simple_string_conds ) {
            if( jo.has_bool( sub_member ) ) {
                dcondition = conditional_t( sub_member );
                function = [dcondition, yes, no]( const dialogue & d ) {
                    return ( dcondition( d ) ? yes : no )( d );
                };
                return;
            } else if( jo.has_member( sub_member ) ) {
                dcondition = conditional_t( sub_member );
                const dynamic_line_t yes_member = from_member( jo, sub_member );
                function = [dcondition, yes_member, no]( const dialogue & d ) {
                    return ( dcondition( d ) ? yes_member : no )( d );
                };
                return;
            }
        }
        for( const std::string &sub_member : dialogue_data::complex_conds ) {
            if( jo.has_member( sub_member ) ) {
                dcondition = conditional_t( jo );
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

void json_talk_topic::load( JsonObject &jo )
{
    if( jo.has_member( "dynamic_line" ) ) {
        dynamic_line = dynamic_line_t::from_member( jo, "dynamic_line" );
    }
    JsonArray ja = jo.get_array( "responses" );
    responses.reserve( responses.size() + ja.size() );
    while( ja.has_more() ) {
        responses.emplace_back( ja.next_object() );
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
    return replace_built_in_responses;
}

std::string json_talk_topic::get_dynamic_line( const dialogue &d ) const
{
    return dynamic_line( d );
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
    //form_opinion(u);
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
consumption_result try_consume( npc &p, item &it, std::string &reason )
{
    // TODO: Unify this with 'player::consume_item()'
    bool consuming_contents = it.is_container() && !it.contents.empty();
    item &to_eat = consuming_contents ? it.contents.front() : it;
    const auto &comest = to_eat.type->comestible;
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
                                        item::nname( comest->tool ).c_str() );
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
        debugmsg( "Unknown comestible type of item: %s\n", to_eat.tname().c_str() );
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
    const int inv_pos = g->inv_for_all( _( "Offer what?" ), _( "You have no items to offer." ) );
    item &given = g->u.i_at( inv_pos );
    if( given.is_null() ) {
        return _( "Changed your mind?" );
    }

    if( &given == &g->u.weapon && given.has_flag( "NO_UNWIELD" ) ) {
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
    long our_ammo = p.ammo_count_for( p.weapon );
    long new_ammo = p.ammo_count_for( given );
    const double new_weapon_value = p.weapon_value( given, new_ammo );
    const double cur_weapon_value = p.weapon_value( p.weapon, our_ammo );
    if( allow_use ) {
        add_msg( m_debug, "NPC evaluates own %s (%d ammo): %0.1f",
                 p.weapon.tname(), our_ammo, cur_weapon_value );
        add_msg( m_debug, "NPC evaluates your %s (%d ammo): %0.1f",
                 given.tname(), new_ammo, new_weapon_value );
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

    std::stringstream reason;
    reason << _( "Nope." );
    reason << std::endl;
    if( allow_use ) {
        if( !no_consume_reason.empty() ) {
            reason << no_consume_reason;
            reason << std::endl;
        }

        reason << _( "My current weapon is better than this." );
        reason << std::endl;
        reason << string_format( _( "(new weapon value: %.1f vs %.1f)." ),
                                 new_weapon_value, cur_weapon_value );
        if( !given.is_gun() && given.is_armor() ) {
            reason << std::endl;
            reason << string_format( _( "It's too encumbering to wear." ) );
        }
    }
    if( allow_carry ) {
        if( !p.can_pickVolume( given ) ) {
            const units::volume free_space = p.volume_capacity() - p.volume_carried();
            reason << std::endl;
            reason << string_format( _( "I have no space to store it." ) );
            reason << std::endl;
            if( free_space > 0_ml ) {
                reason << string_format( _( "I can only store %s %s more." ),
                                         format_volume( free_space ), volume_units_long() );
            } else {
                reason << string_format( _( "...or to store anything else for that matter." ) );
            }
        }
        if( !p.can_pickWeight( given ) ) {
            reason << std::endl;
            reason << string_format( _( "It is too heavy for me to carry." ) );
        }
    }

    return reason.str();
}

bool npc::has_item_whitelist() const
{
    return is_following() && !rules.pickup_whitelist->empty();
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
