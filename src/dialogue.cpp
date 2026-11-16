#include "dialogue.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "auto_pickup.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_imgui.h"
#include "cata_lazy.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character.h"
#include "character_id.h"
#include "clzones.h"
#include "color.h"
#include "condition.h"
#include "coordinates.h"
#include "creature_tracker.h"
#include "debug.h"
#include "dialogue_chatbin.h"
#include "dialogue_helpers.h"
#include "dialogue_imgui.h"
#include "enums.h"
#include "faction.h"
#include "flat_set.h"
#include "flexbuffer_json.h"
#include "game.h"
#include "generic_factory.h"
#include "global_vars.h"
#include "input_context.h"
#include "input_enums.h"
#include "item.h"
#include "item_category.h"
#include "itype.h"
#include "magic.h"
#include "map.h"
#include "martialarts.h"
#include "math_parser_diag_value.h"
#include "messages.h"
#include "mission.h"
#include "npc.h"
#include "npc_opinion.h"
#include "npctrade.h"
#include "output.h"
#include "pimpl.h"
#include "player_activity.h"
#include "point.h"
#include "proficiency.h"
#include "recipe_groups.h"
#include "rng.h"
#include "skill.h"
#include "sounds.h"
#include "string_formatter.h"
#include "talker.h"
#include "text_snippets.h"
#include "translation.h"
#include "translation_gendered.h"
#include "translations.h"
#include "ui_manager.h"
#include "uistate.h"

static const activity_id ACT_AIM( "ACT_AIM" );
static const activity_id ACT_SOCIALIZE( "ACT_SOCIALIZE" );
static const activity_id ACT_TRAIN( "ACT_TRAIN" );
static const activity_id ACT_WAIT_NPC( "ACT_WAIT_NPC" );

static const efftype_id effect_narcosis( "narcosis" );
static const efftype_id effect_under_operation( "under_operation" );

static const skill_id skill_firstaid( "firstaid" );

static const skill_id skill_speech( "speech" );

static const trait_id trait_DEBUG_MIND_CONTROL( "DEBUG_MIND_CONTROL" );
static const trait_id trait_PROF_CHURL( "PROF_CHURL" );

static const zone_type_id zone_type_NPC_INVESTIGATE_ONLY( "NPC_INVESTIGATE_ONLY" );
static const zone_type_id zone_type_NPC_NO_INVESTIGATE( "NPC_NO_INVESTIGATE" );

static std::map<std::string, json_talk_topic> json_talk_topics;

static bool friendly_teacher( const Character &student, const Character &teacher )
{
    return ( student.is_npc() && teacher.is_avatar() ) ||
           ( teacher.is_npc() && teacher.as_npc()->is_player_ally() );
}

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

// Rescale values from "mission scale" to "opinion scale"
int npc_trading::cash_to_favor( const npc &, int cash )
{
    // TODO: It should affect different NPCs to a different degree
    // Square root of mission value in dollars
    // ~31 for zed mom, 50 for horde master, ~63 for plutonium cells
    double scaled_mission_val = std::sqrt( cash / 100.0 );
    return roll_remainder( scaled_mission_val );
}

void npc::handle_sound( const sounds::sound_t spriority, const std::string &description,
                        int heard_volume, const tripoint_bub_ms &spos )
{
    const map &here = get_map();
    const tripoint_abs_ms s_abs_pos = here.get_abs( spos );
    const tripoint_abs_ms my_abs_pos = pos_abs();

    add_msg_debug( debugmode::DF_NPC,
                   "%s heard '%s', priority %d at volume %d from %d:%d, my pos %d:%d",
                   disp_name(), description, static_cast<int>( spriority ), heard_volume,
                   s_abs_pos.x(), s_abs_pos.y(), my_abs_pos.x(), my_abs_pos.y() );

    Character &player_character = get_player_character();
    bool player_ally = player_character.pos_bub() == spos && is_player_ally();
    Character *const sound_source = get_creature_tracker().creature_at<Character>( spos );
    bool npc_ally = sound_source && sound_source->is_npc() && is_ally( *sound_source );

    if( ( player_ally || npc_ally ) && spriority == sounds::sound_t::order ) {
        say( chat_snippets().snip_acknowledged.translated() );
    }

    if( sees( here, spos ) || is_hallucination() ) {
        return;
    }
    // ignore low priority sounds if the NPC "knows" it came from a friend.
    // TODO: NPC will need to respond to talking noise eventually
    // but only for bantering purposes, not for investigating.
    if( spriority < sounds::sound_t::alarm ) {
        if( player_ally ) {
            add_msg_debug( debugmode::DF_NPC, "Allied NPC ignored same faction %s", get_name() );
            return;
        }
        if( npc_ally ) {
            add_msg_debug( debugmode::DF_NPC, "NPC ignored same faction %s", get_name() );
            return;
        }
    }
    // discount if sound source is player, or seen by player,
    // and listener is friendly and sound source is combat or alert only.
    if( spriority < sounds::sound_t::alarm && player_character.sees( here, spos ) ) {
        if( is_player_ally() ) {
            add_msg_debug( debugmode::DF_NPC, "NPC %s ignored low priority noise that player can see",
                           get_name() );
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
                !has_faction_relationship( *sound_source, npc_factions::relationship::knows_your_voice ) ) {
                warn_about( "speech_noise", rng( 1, 10 ) * 1_minutes );
            } else if( spriority > sounds::sound_t::activity ) {
                warn_about( "combat_noise", rng( 1, 10 ) * 1_minutes );
            }
            bool should_check = rl_dist( pos_bub(), spos ) < investigate_dist;
            if( should_check ) {
                const zone_manager &mgr = zone_manager::get_manager();
                // NOLINTNEXTLINE(bugprone-branch-clone)
                if( mgr.has( zone_type_NPC_NO_INVESTIGATE, s_abs_pos, fac_id ) ) {
                    should_check = false;
                } else if( mgr.has( zone_type_NPC_INVESTIGATE_ONLY, my_abs_pos, fac_id ) &&
                           !mgr.has( zone_type_NPC_INVESTIGATE_ONLY, s_abs_pos, fac_id ) ) {
                    should_check = false;
                }
            }
            if( should_check ) {
                add_msg_debug( debugmode::DF_NPC, "%s added noise at pos %d:%d", get_name(),
                               s_abs_pos.x(), s_abs_pos.y() );
                dangerous_sound temp_sound;
                temp_sound.abs_pos = s_abs_pos;
                temp_sound.volume = heard_volume;
                temp_sound.type = spriority;
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

std::string dialog_helper::bye_message( const npc *npc_actor )
{
    // some dialogues do not have beta actor
    if( !npc_actor ) {
        return "";
    }

    return npc_actor->chat_snippets().snip_bye.translated();
}

void avatar::talk_to( std::unique_ptr<talker> talk_with, bool radio_contact,
                      bool is_computer, bool is_not_conversation, const std::string &debug_topic,
                      const std::string &remote_name )
{
    const bool has_mind_control = has_trait( trait_DEBUG_MIND_CONTROL );
    const bool force_topic = !debug_topic.empty();
    if( !talk_with->will_talk_to_u( *this, has_mind_control || force_topic ) ) {
        return;
    }
    dialogue d( get_talker_for( *this ), std::move( talk_with ), {} );
    d.by_radio = radio_contact;
    dialogue_by_radio = radio_contact;
    dialogue_remote_name = remote_name;
    d.actor( true )->check_missions();
    for( mission *&mission : d.actor( true )->assigned_missions() ) {
        if( mission->get_assigned_player_id() == getID() ) {
            d.missions_assigned.push_back( mission );
        }
    }
    if( !force_topic ) {
        for( const std::string &topic_id : d.actor( true )->get_topics( radio_contact ) ) {
            d.add_topic( topic_id );
        }
        for( const std::string &topic_id : d.actor( true )->get_topics( radio_contact ) ) {
            d.add_topic( topic_id );
        }
    } else {
        d.add_topic( debug_topic );
    }
    dialogue_imgui d_img( &d );
    d_img.draw_dialogue_imgui( is_computer, is_not_conversation, remote_name );
    if( activity.id() == ACT_AIM && !has_weapon() ) {
        cancel_activity();
        // don't query certain activities that are started from dialogue
    } else if( activity.id() == ACT_TRAIN || activity.id() == ACT_WAIT_NPC ||
               activity.id() == ACT_SOCIALIZE || activity.index == d.actor( true )->getID().get_value() ) {
        return;
    }

    if( uistate.distraction_conversation &&
        !d.actor( true )->has_effect( effect_under_operation, bodypart_str_id::NULL_ID() ) ) {
        g->cancel_activity_or_ignore_query( distraction_type::talked_to,
                                            string_format( _( "%s talked to you." ),
                                                    d.actor( true )->disp_name() ) );
    }
}

std::string dialogue::speaker_name( const dialogue_imgui_impl &d_img ) const
{
    if( d_img.is_not_conversation ) {
        return "";
    }
    if( !d_img.remote_name.empty() ) {
        return d_img.remote_name;
    }
    return actor( true )->disp_name();
}

std::string dialogue::dynamic_line( const talk_topic &the_topic )
{
    if( !the_topic.item_type.is_null() ) {
        cur_item = the_topic.item_type;
    }

    // For compatibility
    const std::string &topic = the_topic.id;
    const auto iter = json_talk_topics.find( topic );
    if( iter != json_talk_topics.end() ) {
        std::string line = iter->second.get_dynamic_line( *this );
        if( !line.empty() ) {
            return line;
        }
    }

    if( topic == "TALK_NPC_NOFACE" ) {
        return string_format( _( "&%s stays silent." ), actor( true )->disp_name() );
    }

    if( topic == "TALK_NOFACE" ) {
        return _( "&You can't talk without your face." );
    } else if( topic == "TALK_DEAF" ) {
        return _( "&You are deaf and can't talk." );

    } else if( topic == "TALK_DEAF_ANGRY" ) {
        return string_format(
                   _( "&You are deaf and can't talk.  When you don't respond, %s becomes angry!" ),
                   actor( true )->disp_name() );
    } else if( topic == "TALK_MUTE" ) {
        return _( "&You are mute and can't talk." );

    } else if( topic == "TALK_MUTE_ANGRY" ) {
        return string_format(
                   _( "&You are mute and can't talk.  When you don't respond, %s becomes angry!" ),
                   actor( true )->disp_name() );
    } else if( topic == "TALK_CHURL" ) {
        return string_format(
                   _( "&Thou art but a lowley churl and ye know not this newe tongue.  %s seems unable to understand what you're saying." ),
                   actor( true )->disp_name() );

    } else if( topic == "TALK_CHURL_ANGRY" ) {
        return string_format(
                   _( "&Thou art but a lowley churl and ye know not this newe tongue.  Unable to understand your dialect, %s becomes angry!" ),
                   actor( true )->disp_name() );
    } else if( topic == "TALK_CHURL_TRADE" ) {
        return string_format(
                   _( "&Thou art but a lowley churl wyth litel understonding of this newe langage, yet %s can understand you and seems willing to trade!" ),
                   actor( true )->disp_name() );
    }
    avatar &player_character = get_avatar();
    if( topic == "TALK_SEDATED" ) {
        return string_format( _( "%1$s is sedated and can't be moved or woken up until the "
                                 "medication or sedation wears off.\nYou estimate it will wear "
                                 "off in %2$s." ),
                              actor( true )->disp_name(),
                              to_string_approx( player_character.estimate_effect_dur( skill_firstaid,
                                                effect_narcosis, 90_minutes, 60_minutes, 6,
                                                *actor( true )->get_npc() ) ) );
    }

    // Those topics are handled by the mission system, see there.
    static const std::unordered_set<std::string> mission_topics = { {
            "TALK_MISSION_DESCRIBE", "TALK_MISSION_DESCRIBE_URGENT",
            "TALK_MISSION_OFFER", "TALK_MISSION_ACCEPTED",
            "TALK_MISSION_REJECTED", "TALK_MISSION_ADVICE", "TALK_MISSION_INQUIRE",
            "TALK_MISSION_SUCCESS", "TALK_MISSION_SUCCESS_LIE", "TALK_MISSION_FAILURE"
        }
    };
    if( mission_topics.count( topic ) > 0 ) {
        mission *miss = actor( true )->selected_mission();
        if( miss == nullptr ) {
            return "mission_selected == nullptr; BUG!  (npctalk.cpp:dynamic_line)";
        }
        const mission_type &type = miss->get_type();
        // TODO: make it a member of the mission class, maybe at mission instance specific data
        const std::string &ret = miss->dialogue_for_topic( topic );
        if( ret.empty() ) {
            debugmsg( "Bug in npctalk.cpp:dynamic_line.  Wrong mission_id(%s) or topic(%s)",
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
        return dialog_helper::bye_message( actor( true )->get_npc() );
    } else if( topic == "TALK_TRAIN" ) {
        if( !player_character.backlog.empty() && player_character.backlog.front().id() == ACT_TRAIN ) {
            return _( "Shall we resume?" );
        } else if( !actor( false )->can_see() ) {
            return _( "Sorry, I'm not sure how to teach the blind." );
        } else if( actor( true )->skills_offered_to( *actor( false ) ).empty() &&
                   actor( true )->styles_offered_to( *actor( false ) ).empty() &&
                   actor( true )->spells_offered_to( *actor( false ) ).empty() &&
                   actor( true )->proficiencies_offered_to( *actor( false ) ).empty() ) {
            return _( "Sorry, but it doesn't seem I have anything to teach you." );
        } else {
            return _( "Here's what I can teach you…" );
        }
    } else if( topic == "TALK_TRAIN_NPC" ) {
        if( actor( false )->skills_offered_to( *actor( true ) ).empty() &&
            actor( false )->styles_offered_to( *actor( true ) ).empty() &&
            actor( false )->spells_offered_to( *actor( true ) ).empty() &&
            actor( false )->proficiencies_offered_to( *actor( true ) ).empty() ) {
            return _( "Sorry, there's nothing I can learn from you." );
        } else {
            return _( "Sure, I'm all ears." );
        }
    } else if( topic == "TALK_TRAIN_SEMINAR" ) {
        return _( "What do you want me to teach?" );
    } else if( topic == "TALK_HOW_MUCH_FURTHER" ) {
        return actor( true )->distance_to_goal();
    } else if( topic == "TALK_DESCRIBE_MISSION" ) {
        return actor( true )->get_job_description();
    } else if( topic == "TALK_SHOUT" ) {
        actor( false )->shout();
        if( actor( false )->is_deaf() ) {
            return _( "&You yell, but can't hear yourself." );
        } else {
            if( actor( false )->is_mute() ) {
                return _( "&You yell, but can't form words." );
            } else {
                return _( "&You yell." );
            }
        }
    } else if( topic == "TALK_MIND_CONTROL" ) {
        if( actor( true )->enslave_mind() ) {
            return _( "YES, MASTER!" );
        }
    }

    debugmsg( "I don't know what to say for %s. (BUG (npctalk.cpp:dynamic_line))", topic );
    return "";
}

void dialogue::apply_speaker_effects( const talk_topic &the_topic )
{
    const std::string &topic = the_topic.id;
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

std::optional<character_portrait_id> dialogue::portrait_or_nullopt()
const
{
    if( !topic_stack.empty() ) {
        const std::string &topic = topic_stack.back().id;
        const auto iter = json_talk_topics.find( topic );
        if( iter != json_talk_topics.end() && iter->second.get_portrait_override().has_value() ) {
            return iter->second.get_portrait_override();
        }
    }

    Character *partner = actor( true )->get_character();
    if( partner ) {
        return partner->portrait_filename;
    }

    return std::nullopt;
}

talk_response &dialogue::add_response( const std::string &text, const std::string &r,
                                       const bool first )
{
    talk_response result = talk_response();
    result.truetext = no_translation( text );
    result.truefalse_condition = []( const_dialogue const & ) {
        return true;
    };
    result.success.next_topic = talk_topic( r );
    if( first ) {
        add_gen_response( result, true );
        return responses.front();
    } else {
        add_gen_response( result, false );
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
                                       const talkfunction_ptr &effect_success, const bool first )
{
    talk_response &result = add_response( text, r, first );
    result.success.set_effect( effect_success );
    return result;
}

talk_response &dialogue::add_response( const std::string &text, const std::string &r,
                                       const std::function<void( npc & )> &effect_success,
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
                                       const proficiency_id &proficiency, const bool first )
{
    talk_response &result = add_response( text, r, first );
    result.proficiency = proficiency;
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
    if( item_type.is_null() ) {
        debugmsg( "explicitly specified null item" );
    }

    talk_response &result = add_response( text, r, first );
    result.success.next_topic.item_type = item_type;
    return result;
}

void dialogue::gen_responses( const talk_topic &the_topic )
{
    responses.clear();
    response_condition_exists.clear();
    response_condition_eval.clear();

    const auto iter = json_talk_topics.find( the_topic.id );
    if( iter != json_talk_topics.end() ) {
        json_talk_topic &jtt = iter->second;
        if( jtt.gen_responses( *this ) ) {
            return;
        }
    }

    Character &player_character = get_player_character();
    if( the_topic.id == "TALK_MISSION_LIST" ) {
        if( actor( true )->available_missions().size() == 1 ) {
            add_response( _( "Tell me about it." ), "TALK_MISSION_OFFER",
                          actor( true )->available_missions().front(), true );
        } else {
            for( mission *&mission : actor( true )->available_missions() ) {
                add_response( mission->get_type().tname(), "TALK_MISSION_OFFER", mission, true );
            }
        }
    } else if( the_topic.id == "TALK_MISSION_LIST_ASSIGNED" ) {
        if( missions_assigned.size() == 1 ) {
            add_response( _( "I have news." ), "TALK_MISSION_INQUIRE", missions_assigned.front() );
        } else {
            for( mission *&miss_it : missions_assigned ) {
                add_response( miss_it->get_type().tname(), "TALK_MISSION_INQUIRE", miss_it );
            }
        }
    } else if( the_topic.id == "TALK_TRAIN_NPC" ) {
        const std::vector<matype_id> &styles = actor( false )->styles_offered_to( *actor( true ) );
        const std::vector<skill_id> &skills = actor( false )->skills_offered_to( *actor( true ) );
        const std::vector<spell_id> &spells = actor( false )->spells_offered_to( *actor( true ) );
        const std::vector<proficiency_id> &profs =
            actor( false )->proficiencies_offered_to( *actor( true ) );
        if( skills.empty() && styles.empty() && spells.empty() && profs.empty() ) {
            add_response_none( _( "Oh, okay." ) );
            return;
        }
        if( !actor( false )->can_see() ) {
            add_response_none( _( "You're not sure how to teach them while blind." ) );
            return;
        }
        for( const spell_id &sp : spells ) {
            const std::string &text =
                string_format( "%s: %s", _( "Spell" ), actor( false )->spell_training_text( *actor( true ), sp ) );
            if( !text.empty() ) {
                add_response( text, "TALK_TRAIN_NPC_START", sp );
            }
        }
        for( const matype_id &ma : styles ) {
            const std::string &text =
                string_format( "%s: %s", _( "Style" ), actor( false )->style_training_text( *actor( true ), ma ) );
            if( !text.empty() ) {
                add_response( text, "TALK_TRAIN_NPC_START", ma.obj() );
            }
        }
        for( const skill_id &sk : skills ) {
            const std::string &text =
                string_format( "%s: %s", _( "Skill" ), actor( false )->skill_training_text( *actor( true ), sk ) );
            if( !text.empty() && !sk->obsolete() ) {
                add_response( text, "TALK_TRAIN_NPC_START", sk );
            }
        }
        for( const proficiency_id &pr : profs ) {
            const std::string &text =
                string_format( "%s: %s", _( "Proficiency" ),
                               actor( false )->proficiency_training_text( *actor( true ), pr ) );
            if( !text.empty() ) {
                add_response( text, "TALK_TRAIN_NPC_START", pr );
            }
        }
        add_response_none( _( "Eh, never mind." ) );
    } else if( the_topic.id == "TALK_TRAIN_SEMINAR" ) {
        const std::vector<skill_id> &sklist = actor( true )->skills_teacheable();
        const std::vector<proficiency_id> &prlist = actor( true )->proficiencies_teacheable();
        const std::vector<matype_id> &malist = actor( true )->styles_teacheable();
        const std::vector<spell_id> &splist = actor( true )->spells_teacheable();
        if( sklist.empty() && prlist.empty() && malist.empty() && splist.empty() ) {
            add_response_none( _( "Oh, okay." ) );
            return;
        }
        for( const skill_id &sk : sklist ) {
            if( sk->obsolete() ) {
                continue;
            }
            const std::string &text =
                string_format( "%s: %s", _( "Skill" ), actor( true )->skill_seminar_text( sk ) );
            add_response( text, "TALK_TRAIN_SEMINAR_START", sk );
        }
        for( const proficiency_id &pr : prlist ) {
            const std::string &text =
                string_format( "%s: %s", _( "Proficiency" ), actor( true )->proficiency_seminar_text( pr ) );
            add_response( text, "TALK_TRAIN_SEMINAR_START", pr );
        }
        for( const matype_id &ma : malist ) {
            const std::string &text =
                string_format( "%s: %s", _( "Style" ), actor( true )->style_seminar_text( ma ) );
            add_response( text, "TALK_TRAIN_SEMINAR_START", ma.obj() );
        }
        for( const spell_id &sp : splist ) {
            const std::string &text =
                string_format( "%s: %s", _( "Spell" ), actor( true )->spell_seminar_text( sp ) );
            add_response( text, "TALK_TRAIN_SEMINAR_START", sp );
        }
        add_response_none( _( "Eh, never mind." ) );
    } else if( the_topic.id == "TALK_TRAIN" ) {
        if( !player_character.backlog.empty() && player_character.backlog.front().id() == ACT_TRAIN &&
            player_character.backlog.front().index == actor( true )->getID().get_value() ) {
            player_activity &backlog = player_character.backlog.front();
            const skill_id skillt( backlog.name );
            // TODO: This is potentially dangerous. A skill and a martial art
            // could have the same ident!
            if( !skillt.is_valid() ) {
                const matype_id styleid = matype_id( backlog.name );
                if( !styleid.is_valid() ) {
                    const proficiency_id profid = proficiency_id( backlog.name );
                    if( !profid.is_valid() ) {
                        const spell_id &sp_id = spell_id( backlog.name );
                        if( actor( true )->knows_spell( sp_id ) ) {
                            add_response( string_format( _( "Yes, let's resume training %s" ),
                                                         sp_id->name ), "TALK_TRAIN_START", sp_id );
                        }
                    } else {
                        add_response( string_format( _( "Yes, let's resume training %s" ),
                                                     profid->name() ), "TALK_TRAIN_START", profid );
                    }
                } else {
                    const martialart &style = styleid.obj();
                    add_response( string_format( _( "Yes, let's resume training %s" ),
                                                 style.name ), "TALK_TRAIN_START", style );
                }
            } else {
                add_response( string_format( _( "Yes, let's resume training %s" ), skillt->name() ),
                              "TALK_TRAIN_START", skillt );
            }
        }

        if( !actor( false )->can_see() ) {
            add_response_none( _( "Oh, okay." ) );
            return;
        }
        const std::vector<matype_id> &styles = actor( true )->styles_offered_to( *actor( false ) );
        const std::vector<skill_id> &trainable = actor( true )->skills_offered_to( *actor( false ) );
        const std::vector<spell_id> &teachable = actor( true )->spells_offered_to( *actor( false ) );
        const std::vector<proficiency_id> &proficiencies = actor( true )->proficiencies_offered_to( *actor(
                    false ) );
        if( trainable.empty() && styles.empty() && teachable.empty() && proficiencies.empty() ) {
            add_response_none( _( "Oh, okay." ) );
            return;
        }
        for( const spell_id &sp : teachable ) {
            const std::string &text =
                string_format( "%s: %s", _( "Spell" ), actor( true )->spell_training_text( *actor( false ), sp ) );
            if( !text.empty() ) {
                add_response( text, "TALK_TRAIN_START", sp );
            }
        }
        for( const matype_id &style_id : styles ) {
            const std::string &text =
                string_format( "%s: %s", _( "Style" ),
                               actor( true )->style_training_text( *actor( false ), style_id ) );
            if( !text.empty() ) {
                add_response( text, "TALK_TRAIN_START", style_id.obj() );
            }
        }
        for( const skill_id &trained : trainable ) {
            const std::string &text =
                string_format( "%s: %s", _( "Skill" ),
                               actor( true )->skill_training_text( *actor( false ), trained ) );
            if( !text.empty() && !trained->obsolete() ) {
                add_response( text, "TALK_TRAIN_START", trained );
            }
        }
        for( const proficiency_id &trained : proficiencies ) {
            const std::string &text =
                string_format( "%s: %s", _( "Proficiency" ),
                               actor( true )->proficiency_training_text( *actor( false ), trained ) );
            if( !text.empty() ) {
                add_response( text, "TALK_TRAIN_START", trained );
            }
        }
        add_response_none( _( "Eh, never mind." ) );
    } else if( the_topic.id == "TALK_HOW_MUCH_FURTHER" ) {
        add_response_none( _( "Okay, thanks." ) );
        add_response_done( _( "Let's keep moving." ) );
    }

    if( actor( false )->has_trait( trait_DEBUG_MIND_CONTROL ) && !actor( true )->is_player_ally() ) {
        add_response( _( "OBEY ME!" ), "TALK_MIND_CONTROL" );
        add_response_done( _( "Bye." ) );
    }

    if( player_character.has_trait( trait_PROF_CHURL ) && ( actor( true )->get_npc_trust() >= 0 ) &&
        ( actor( true )->get_npc_anger() <= 0 ) && ( actor( true )->int_cur() >= 9 ) &&
        !( the_topic.id == "TALK_CHURL_FRIENDLY" ) ) {
        add_response( _( "Ho there, otherwyrldly devyl!  Have yow ware for to chaffare?" ),
                      "TALK_CHURL_FRIENDLY" );
        add_response_done( _( "Farewell!" ) );
    }

    if( responses.empty() ) {
        add_response_done( _( "Bye." ) );
    }
}

bool talk_trial::roll( dialogue &d ) const
{
    if( type == TALK_TRIAL_NONE || d.actor( false )->has_trait( trait_DEBUG_MIND_CONTROL ) ) {
        return true;
    }
    const int chance = calc_chance( d );
    const bool success = rng( 0, 99 ) < chance;
    const bool speech_trial = type == TALK_TRIAL_PERSUADE || type == TALK_TRIAL_INTIMIDATE ||
                              type == TALK_TRIAL_LIE;
    if( speech_trial && d.actor( false )->get_character() ) {
        Character &u = *d.actor( false )->get_character();
        if( success ) {
            u.practice( skill_speech, ( 100 - chance ) / 10 );
        } else {
            u.practice( skill_speech, ( 100 - chance ) / 7 );
        }
    }
    return success;
}

void dialogue::add_gen_response( const talk_response &resp, bool insert_front,
                                 bool condition_exists, bool condition_result )
{
    if( insert_front ) {
        responses.insert( responses.begin(), resp );
    } else {
        responses.push_back( resp );
    }
    response_condition_exists.emplace_back( condition_exists );
    response_condition_eval.emplace_back( condition_result );
}

void dialogue::add_topic( const std::string &topic_id )
{
    if( actor( true )->get_npc() ) {
        topic_stack.emplace_back( actor( true )->get_npc()->get_specified_talk_topic( topic_id ) );
    } else {
        topic_stack.emplace_back( topic_id );
    }
}

void dialogue::add_topic( const talk_topic &topic )
{
    if( actor( true )->get_npc() ) {
        std::string const &newid = actor( true )->get_npc()->get_specified_talk_topic( topic.id );
        topic_stack.emplace_back( newid, topic.item_type, topic.reason );
    } else {
        topic_stack.push_back( topic );
    }
}

void const_dialogue::set_value( const std::string &key, diag_value value )
{
    context[key] = std::move( value );
}

void const_dialogue::remove_value( const std::string &key )
{
    context->erase( key );
}

diag_value const &const_dialogue::get_value( const std::string &key ) const
{
    return global_variables::_common_get_value( key, context );
}

diag_value const *const_dialogue::maybe_get_value( const std::string &key ) const
{
    return global_variables::_common_maybe_get_value( key, context );
}

void const_dialogue::set_conditional( const std::string &key,
                                      const std::function<bool( const_dialogue const & )> &value )
{
    conditionals[key] = value;
}

bool const_dialogue::evaluate_conditional( const std::string &key, const_dialogue const &d ) const
{
    auto it = conditionals->find( key );
    return ( it == conditionals->end() ) ? false : it->second( d );
}

global_variables::impl_t const &const_dialogue::get_context() const
{
    return context;
}

const std::unordered_map<std::string, std::function<bool( const_dialogue const & )>>
        &const_dialogue::get_conditionals() const
{
    return conditionals;
}

void const_dialogue::amend_callstack( const std::string &value )
{
    if( !callstack.empty() ) {
        callstack += " \\ " + value;
    } else {
        callstack = value;
    }
}

std::string const_dialogue::get_callstack() const
{
    if( !callstack.empty() ) {
        return "Callstack: " + callstack;
    }
    return "";
}

const_talker *const_dialogue::const_actor( bool is_beta ) const
{
    if( !has_beta && is_beta ) {
        debugmsg( "Tried to use an invalid beta talker.  %s", get_callstack() );
    } else if( !has_alpha && !is_beta ) {
        debugmsg( "Tried to use an invalid alpha talker.  %s", get_callstack() );
    }
    return ( is_beta ? beta : alpha ).get();
}

talker *dialogue::actor( const bool is_beta ) const
{
    if( !has_beta && !has_alpha ) {
        debugmsg( "Attempted to use a dialogue with no actors!  %s", get_callstack() );
    }
    if( is_beta && !has_beta ) {
        debugmsg( "Tried to use an invalid beta talker.  %s", get_callstack() );
        // Try to avoid a crash by using the alpha if it exists
        if( has_alpha ) {
            return alpha.get();
        }
    }
    if( !is_beta && !has_alpha ) {
        debugmsg( "Tried to use an invalid alpha talker.  %s", get_callstack() );
        // Try to avoid a crash by using the beta if it exists
        if( has_beta ) {
            return beta.get();
        }
    }
    return ( is_beta ? beta : alpha ).get();
}

bool const_dialogue::has_actor( bool is_beta ) const
{
    return is_beta ? has_beta : has_alpha;
}

const_dialogue::const_dialogue( const const_dialogue &d )
    : has_beta( d.has_beta ), has_alpha( d.has_alpha )
{
    if( has_alpha ) {
        alpha = d.const_actor( false )->const_clone();
    }
    if( has_beta ) {
        beta = d.const_actor( true )->const_clone();
    }
    if( !has_alpha && !has_beta ) {
        debugmsg( "Constructed a dialogue with no actors!  %s", get_callstack() );
    }
    if( d.context.has_value() ) {
        context = *d.context;
    }
    if( d.conditionals.has_value() ) {
        conditionals = *d.conditionals;
    }
    callstack = d.callstack;
}

dialogue::dialogue( const dialogue &d ) : const_dialogue( d )
{
    if( has_alpha ) {
        alpha = d.actor( false )->clone();
    }
    if( has_beta ) {
        beta = d.actor( true )->clone();
    }
}

const_dialogue::const_dialogue( std::unique_ptr<const_talker> alpha_in,
                                std::unique_ptr<const_talker> beta_in,
                                const std::unordered_map<std::string, std::function<bool( const_dialogue const & )>> &cond,
                                global_variables::impl_t const &ctx )
    : alpha( std::move( alpha_in ) ), beta( std::move( beta_in ) ), context( ctx ), conditionals( cond )
{
    has_alpha = static_cast<bool>( alpha );
    has_beta = static_cast<bool>( beta );
}

dialogue::dialogue(
    std::unique_ptr<talker> alpha_in, std::unique_ptr<talker> beta_in,
    const std::unordered_map<std::string, std::function<bool( const_dialogue const & )>> &cond,
    global_variables::impl_t const &ctx )
    : const_dialogue( alpha_in ? alpha_in->const_clone() : nullptr,
                      beta_in ? beta_in->const_clone() : nullptr, cond, ctx ),
      alpha( std::move( alpha_in ) ), beta( std::move( beta_in ) )
{
}

talk_data talk_response::create_option_line( dialogue &d, const input_event &hotkey,
        const bool is_computer )
{
    std::string ftext;
    text = ( truefalse_condition( d ) ? truetext : falsetext ).translated();
    if( trial.type == TALK_TRIAL_NONE || trial.type == TALK_TRIAL_CONDITION ) {
        // regular dialogue
        ftext = text;
    } else if( trial.type == TALK_TRIAL_SKILL_CHECK ) {
        const Skill &req_skill = skill_id( trial.skill_required ).obj();
        ftext = string_format( pgettext( "talk option", "[%1$s %2$d/%3$d] %4$s" ),
                               req_skill.name(),
                               std::min( d.actor( false )->get_skill_level( req_skill.ident() ),
                                         trial.difficulty ),
                               trial.difficulty,
                               text );
    } else {
        // dialogue w/ a % chance to work
        //~ %1$s is translated trial type, %2$d is a number, and %3$s is the translated response text
        ftext = string_format( pgettext( "talk option", "[%1$s %2$d%%] %3$s" ),
                               trial.name(), trial.calc_chance( d ), text );
    }

    if( ignore_conditionals ) {
        ftext = colorize( ftext, c_dark_gray );
        if( !show_reason.empty() ) {
            ftext += colorize( " -- [" + show_reason + "]", c_light_red );
        }
    }

    if( d.actor( true )->get_npc() ) {
        parse_tags( ftext, *d.actor( false )->get_character(), *d.actor( true )->get_npc(), d,
                    success.next_topic.item_type );
    } else {
        parse_tags( ftext, *d.actor( false )->get_character(), *d.actor( false )->get_character(), d,
                    success.next_topic.item_type );
    }

    nc_color color;
    std::set<dialogue_consequence> consequences = get_consequences( d );
    if( consequences.count( dialogue_consequence::hostile ) > 0 ) {
        color = c_red;
    } else if( text[0] == '*' || consequences.count( dialogue_consequence::helpless ) > 0 ) {
        color = c_light_red;
    } else if( text[0] == '&' || consequences.count( dialogue_consequence::action ) > 0 ||
               is_computer ) {
        color = c_green;
    } else {
        color = c_white;
    }
    talk_data results;
    results.color = color;
    results.hotkey_desc = right_justify( hotkey.short_description(), 2 );
    results.text = ftext;
    return results;
}

std::set<dialogue_consequence> talk_response::get_consequences( dialogue &d ) const
{
    int chance = trial.calc_chance( d );
    if( chance >= 100 ) {
        return { success.get_consequence( d ) };
    } else if( chance <= 0 ) {
        return { failure.get_consequence( d ) };
    }

    return {{ success.get_consequence( d ), failure.get_consequence( d ) }};
}

bool json_talk_response::show_anyways( dialogue &d ) const
{
    return actual_response.show_always || ( actual_response.show_condition &&
                                            actual_response.show_condition( d ) );
}

dialogue_consequence talk_effect_t::get_consequence( dialogue const &d ) const
{
    if( d.actor( true )->check_hostile_response( opinion.anger ) ) {
        return dialogue_consequence::hostile;
    }
    return guaranteed_consequence;
}

talk_topic dialogue::opt_imgui( dialogue_imgui_impl &d_img, const talk_topic &topic,
                                input_context &ctxt )
{
    // Construct full line
    std::string challenge = dynamic_line( topic );
    gen_responses( topic );

    // Put quotes around challenge (unless it's an action)
    if( challenge[0] != '*' && challenge[0] != '&' ) {
        challenge = string_format( _( "\"%s\"" ), challenge );
    }

    // Parse any tags in challenge
    if( actor( true )->get_npc() ) {
        parse_tags( challenge, *actor( false )->get_character(), *actor( true )->get_npc(), *this,
                    topic.item_type );
    } else {
        parse_tags( challenge, *actor( false )->get_character(), *actor( false )->get_character(), *this,
                    topic.item_type );
    }
    challenge = uppercase_first_letter( challenge );

    if( challenge[0] == '&' ) {
        // No name prepended!
        challenge = challenge.substr( 1 );
        d_img.add_to_history( challenge );
    } else if( challenge[0] == '*' ) {
        // Prepend name
        challenge = string_format( pgettext( "npc does something", "%s %s" ),
                                   speaker_name( d_img ),
                                   challenge.substr( 1 ) );
        d_img.add_to_history( challenge );
    } else {
        npc *npc_actor = actor( true )->get_npc();
        d_img.add_to_history( challenge, speaker_name( d_img ),
                              npc_actor ? npc_actor->basic_symbol_color() : c_red );
        // Empty line for padding. This is right after either a NPC greeting the player, or them responding to a player's message.
        // So padding here keeps messages grouped into reasonable blocks.
        d_img.add_to_history( "" );
    }
    if( debug_mode ) {
        std::vector<std::string> dynamic_line_debug = build_debug_info( d_img, topic );
        for( auto &line : dynamic_line_debug ) {
            d_img.add_to_history( line );
        }
    }
    apply_speaker_effects( topic );

    if( responses.empty() ) {
        debugmsg( "No dialogue responses" );
        return talk_topic( "TALK_NONE" );
    }

    std::vector<talk_data> response_lines;
    std::vector<input_event> response_hotkeys;
    const auto generate_response_lines = [&]() {
#if defined(__ANDROID__)
        ctxt.get_registered_manual_keys().clear();
#endif
        const hotkey_queue &queue = hotkey_queue::alphabets();
        response_lines.clear();
        response_hotkeys.clear();
        input_event evt = ctxt.first_unassigned_hotkey( queue );
        const int num_responses = responses.size();
        for( int i = 0; i < num_responses; i++ ) {
            talk_response &response = responses[i];
            const talk_data &td = response.create_option_line( *this, evt, d_img.is_computer );
            response_lines.emplace_back( td );
            response_hotkeys.emplace_back( evt );
#if defined(__ANDROID__)
            ctxt.register_manual_key( evt.get_first_input(), td.text );
#endif
            evt = ctxt.next_unassigned_hotkey( queue, evt );
        }
        d_img.set_responses( response_lines );
    };
    generate_response_lines();

    // HACK: We might have hidden the UI on the last frame. Let's go ahead and unhide it now.
    d_img.hide_ui = false;

    // Re-init everytime we get here (new topic, new selections, don't try to preserve old selection)
    d_img.sel_response = 0;
    size_t response_ind = response_hotkeys.size();
    bool okay;
    do {
        std::string action;
        do {
            if( debug_mode ) {
                d_img.set_responses_debug( build_debug_info( d_img, topic, d_img.sel_response ) );
                d_img.debug_topic_name = topic.id;
            }
            // For reasons unclear to me, we must manually invalidate and redraw the windows here, or else they will stack up.
            ui_manager::invalidate_all_ui_adaptors();
            ui_manager::redraw();
            action = ctxt.handle_input();

            // Mouse click is an input type that would result in a continue, so we need to set our action before that.
            if( action == "CONFIRM" || d_img.user_clicked_response_button ) {
                action = "CONFIRM"; // If we actually did click, harmless otherwise.
                d_img.user_clicked_response_button = false;
                response_ind = d_img.sel_response;
                //response condition must be reverified since non-selectable responses can be displayed
                if( response_condition_exists[response_ind] && ( !response_condition_eval[response_ind] &&
                        !debug_mode ) ) {
                    action = "NONE";
                }
            }

            input_event evt = ctxt.get_raw_input();
            if( evt.type == input_event_t::error || evt.type == input_event_t::timeout ) {
                continue;
            }
            if( action == "HELP_KEYBINDINGS" ) {
                // Reallocate hotkeys as keybindings may have changed
                generate_response_lines();
            } else if( action == "DOWN" ) {
                d_img.sel_response++;
            } else if( action == "UP" ) {
                d_img.sel_response--;
            } else if( action == "END" ) {
                d_img.scroll_to = cataimgui::scroll::page_down;
            } else if( action == "HOME" ) {
                d_img.scroll_to = cataimgui::scroll::page_up;
            } else if( action == "PAGE_UP" ) {
                d_img.scroll_to = cataimgui::scroll::line_up;
            } else if( action == "PAGE_DOWN" ) {
                d_img.scroll_to = cataimgui::scroll::line_down;
            } else if( action == "DEBUG_DIALOGUE_DL_CONDITIONAL" ) {
                d_img.show_dynamic_line_conditionals = !d_img.show_dynamic_line_conditionals;
            } else if( action == "DEBUG_DIALOGUE_RESP_CONDITIONAL" ) {
                d_img.show_response_conditionals = !d_img.show_response_conditionals;
            } else if( action == "DEBUG_DIALOGUE_DL_EFFECT" ) {
                d_img.show_dynamic_line_effects = !d_img.show_dynamic_line_effects;
            } else if( action == "DEBUG_DIALOGUE_RESP_EFFECT" ) {
                d_img.show_response_effects = !d_img.show_response_effects;
            } else if( action == "DEBUG_DIALOGUE_SHOW_ALL_RESPONSE" ) {
                d_img.show_all_responses = !d_img.show_all_responses;
                if( debug_mode ) {
                    this->debug_ignore_conditionals = !this->debug_ignore_conditionals;
                    gen_responses( topic );
                    generate_response_lines();
                }
            } else if( action == "ANY_INPUT" ) {
                // Check real hotkeys; equivalent functionally to CONFIRM
                const auto hotkey_it = std::find( response_hotkeys.begin(),
                                                  response_hotkeys.end(), evt );
                response_ind = std::distance( response_hotkeys.begin(), hotkey_it );
                if( response_condition_exists[response_ind] && ( !response_condition_eval[response_ind] &&
                        !debug_mode ) ) {
                    action = "NONE";
                }
            } else if( action == "QUIT" ) {
                response_ind = get_best_quit_response();
            }

            // Clamp sel_response (There is probably a better existing way to do this, but it's simple so I'm just reimplementing)
            if( d_img.sel_response >= static_cast<int>( response_ind ) ) {
                d_img.sel_response = 0;
            } else if( d_img.sel_response < 0 ) {
                d_img.sel_response = static_cast<int>( response_ind ) - 1;
            }

        } while( response_ind >= response_hotkeys.size() ||
                 ( action != "ANY_INPUT" && action != "QUIT" && action != "CONFIRM" ) );
        okay = true;
        std::set<dialogue_consequence> consequences = responses[response_ind].get_consequences( *this );
        if( consequences.count( dialogue_consequence::hostile ) > 0 ) {
            okay = query_yn( _( "You may be attacked!  Proceed?" ) );
        } else if( consequences.count( dialogue_consequence::helpless ) > 0 ) {
            okay = query_yn( _( "You'll be helpless!  Proceed?" ) );
        }
    } while( !okay );

    d_img.add_to_history( response_lines[response_ind].text, _( "You" ), c_light_blue );

    // We just advanced the conversation, let's make sure we can see what they said.
    d_img.scroll_to = cataimgui::scroll::end;

    talk_response chosen = responses[response_ind];
    if( chosen.mission_selected != nullptr ) {
        actor( true )->select_mission( chosen.mission_selected );
    }

    // We can't set both skill and style or training will bug out
    // TODO: Allow setting both skill and style
    actor( true )->store_chosen_training( chosen.skill, chosen.style, chosen.dialogue_spell,
                                          chosen.proficiency );
    const bool success = chosen.trial.roll( *this );
    talk_effect_t const &effects = success ? chosen.success : chosen.failure;

    // HACK: We don't know what effects just happened! We might be trading! Let's go ahead and hide the UI, just in case.
    // If we *aren't* trading, the next frame this function will just un-hide itself.
    // But if we're in the trade menu, un-hiding only gets called when we're back to talking! Easy.
    d_img.hide_ui = true;
    ui_manager::invalidate_all_ui_adaptors();
    ui_manager::redraw();

    talk_topic ret_topic =  effects.apply( *this );
    talk_effect_t::update_missions( *this );
    return ret_topic;
}

/**
 * Finds the best response to use when the player is trying to quit.
 *
 * Returns the index into the response list.
 */
int dialogue::get_best_quit_response()
{
    if( responses.size() == 1 ) {
        // Only one response. Use it. Consequences will be prompted for by the caller.
        return 0;
    }

    // Find relevant responses
    for( size_t i = 0; i < responses.size(); ++i ) {
        const talk_response &response = responses[i];
        if( response.trial.calc_chance( *this ) < 100 ) {
            // Don't pick anything with a chance to fail.
            continue;
        }

        if( !response.success.effects.empty() ) {
            // Don't pick anything with side effects
            continue;
        }

        // Unfortunately, while we'd like to be able to go "back" from nested dialogue trees, the
        // topic stack doesn't always shrink. Returning to the previous topic is sometimes done
        // with TALK_NONE, or sometimes by referencing the topic id directly. No solution really
        // gives us something that feels right in all cases, so we only support completely leaving
        // the conversation via the quit key.

        if( response.success.next_topic.id == "TALK_DONE" ) {
            return i;
        }
    }

    return responses.size(); // Didn't find a good option
}

void get_raw_debug_fields( const JsonObject &jo, std::map<std::string, std::string> &debug_info )
{
    //check for object, add raw JSON text to map if exists
    auto add_to_debug = [&]( const JsonObject & jo, const char *key ) {
        std::string key_str( key );
        if( jo.has_string( key_str ) ) {
            debug_info.emplace( key_str, jo.get_string( key_str ) );
        } else if( jo.has_object( key_str ) ) {
            JsonObject json_cond = jo.get_object( key_str );
            json_cond.allow_omitted_members();
            debug_info.emplace( key_str, json_cond.str() );
        } else if( jo.has_array( key_str ) ) {
            JsonArray json_cond = jo.get_array( key_str );
            std::string concat;
            for( JsonValue elem : json_cond ) {
                if( elem.test_string() ) {
                    concat += elem.get_string();
                } else if( elem.test_object() ) {
                    JsonObject json_obj = elem.get_object();
                    json_obj.allow_omitted_members();
                    concat += json_obj.str();
                }
            }
            debug_info.emplace( key_str, concat );
        }
    };

    add_to_debug( jo, "condition" );
    add_to_debug( jo, "effect" );
    add_to_debug( jo, "trial" );
    add_to_debug( jo, "success" );
    add_to_debug( jo, "failure" );
}


std::vector<std::string> dialogue::build_debug_info( dialogue_imgui_impl &d_img,
        const talk_topic &topic, int do_response )
{
    std::vector<std::string> debug_output;
    json_talk_topic json_topic = json_talk_topics[topic.id];
    if( do_response > -1 ) {
        std::string write_str;
        std::vector<json_talk_response> json_responses = json_topic.get_responses();
        if( json_responses.empty() ) {
            return debug_output;
        }

        talk_response &actual_response = responses[do_response];
        std::map<std::string, std::string> &debug_info = actual_response.debug_info;
        if( d_img.show_response_conditionals ) {
            if( actual_response.condition && debug_info.find( "condition" ) != debug_info.end() ) {
                debug_output.emplace_back( std::string( "Conditional: [" ) + ( actual_response.condition(
                                               *this ) ? colorize( "true", c_light_green ) : colorize( "false",
                                                       c_light_red ) ) + std::string( "] - " ) + debug_info["condition"] );
            } else {
                debug_output.emplace_back( "No conditionals" );
            }
        }
        if( debug_info.find( "trial" ) != debug_info.end() ) {
            if( d_img.show_response_conditionals ) {
                if( actual_response.trial.condition ) {
                    debug_output.emplace_back( std::string( "Trial: [" ) + ( actual_response.trial.condition(
                                                   *this ) ? colorize( "true", c_light_green ) : colorize( "false",
                                                           c_light_red ) ) + std::string( "] - " ) + debug_info["trial"] );
                } else {
                    debug_output.emplace_back( "Trial: " + debug_info["trial"] );
                }
            }
            if( d_img.show_response_effects ) {
                debug_output.emplace_back( colorize( "Success: ", c_green ) + debug_info["success"] );
                debug_output.emplace_back( colorize( "Failure: ", c_red ) + debug_info["failure"] );
            }
        }
        if( d_img.show_response_effects ) {
            if( debug_info.find( "effect" ) != debug_info.end() ) {
                debug_output.emplace_back( "Effect: " + debug_info["effect"] );
            } else {
                debug_output.emplace_back( "No effects" );
            }
        }
    } else {
        std::vector<json_dynamic_line_effect> speaker_effects = json_topic.get_speaker_effects();
        int eff_count = 1;
        bool added_cond = false;
        bool added_eff = false;
        for( json_dynamic_line_effect eff : speaker_effects ) {
            std::map<std::string, std::string> &debug_info = eff.debug_info;
            std::string eff_count_str = std::to_string( eff_count );
            if( debug_info.find( "condition" ) != debug_info.end() && d_img.show_dynamic_line_conditionals ) {
                debug_output.emplace_back( colorize( "CND" + eff_count_str + std::string( ": [" ),
                                                     c_yellow ) + ( eff.test_condition(
                                                             *this ) ? colorize( "true", c_light_green ) : colorize( "false",
                                                                     c_light_red ) ) + std::string( "] - " ) + colorize( debug_info["condition"], c_yellow ) );
                added_cond = true;
            }
            if( debug_info.find( "effect" ) != debug_info.end() && d_img.show_dynamic_line_effects ) {
                debug_output.emplace_back( colorize( "EFF" + eff_count_str + ": " + debug_info["effect"],
                                                     c_yellow ) );
                added_eff = true;
            }
            eff_count++;
        }
        if( !added_cond && d_img.show_dynamic_line_conditionals ) {
            debug_output.emplace_back( colorize( "No conditionals", c_yellow ) );
        }
        if( !added_eff && d_img.show_dynamic_line_effects ) {
            debug_output.emplace_back( colorize( "No effects", c_yellow ) );
        }
    }

    return debug_output;
}

talk_trial::talk_trial( const JsonObject &jo )
{
    static const std::unordered_map<std::string, talk_trial_type> types_map = { {
#define WRAP(value) { #value, TALK_TRIAL_##value }
            WRAP( NONE ),
            WRAP( LIE ),
            WRAP( PERSUADE ),
            WRAP( INTIMIDATE ),
            WRAP( SKILL_CHECK ),
            WRAP( CONDITION )
#undef WRAP
        }
    };
    const auto iter = types_map.find( jo.get_string( "type", "NONE" ) );
    if( iter == types_map.end() ) {
        jo.throw_error_at( "type", "invalid talk trial type" );
    }
    type = iter->second;
    if( type != TALK_TRIAL_NONE && type != TALK_TRIAL_CONDITION ) {
        difficulty = jo.get_int( "difficulty" );
    }
    if( type == TALK_TRIAL_SKILL_CHECK ) {
        skill_required = jo.get_string( "skill_required" );
    }
    //condition remains empty instead of adding a default
    if( jo.has_member( "condition" ) ) {
        read_condition( jo, "condition", condition, false );
    }
    if( jo.has_member( "mod" ) ) {
        for( JsonArray jmod : jo.get_array( "mod" ) ) {
            trial_mod this_modifier;
            this_modifier.first = jmod.next_string();
            this_modifier.second = jmod.next_int();
            modifiers.push_back( this_modifier );
        }
    }
}

static talk_topic load_inline_topic( const JsonObject &jo, std::string_view src )
{
    const std::string id = jo.get_string( "id" );
    json_talk_topics[id].load( jo, src );
    return talk_topic( id );
}

void talk_effect_t::set_effect_consequence( const talk_effect_fun_t &fun,
        dialogue_consequence con )
{
    effects.push_back( fun );
    guaranteed_consequence = std::max( guaranteed_consequence, con );
}

void talk_effect_t::set_effect_consequence( const std::function<void( npc &p )> &ptr,
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

talk_topic talk_effect_t::apply( dialogue &d ) const
{
    if( d.has_beta ) {
        // Need to get a reference to the mission before effects are applied, because effects can remove the mission
        const mission *miss = d.actor( true )->selected_mission();
        for( const talk_effect_fun_t &effect : effects ) {
            effect( d );
        }
        d.actor( true )->add_opinion( opinion );
        if( miss && ( mission_opinion.trust || mission_opinion.fear ||
                      mission_opinion.value || mission_opinion.anger ) ) {
            const int m_value = d.actor( true )->cash_to_favor( miss->get_value() );
            npc_opinion op;
            op.trust = mission_opinion.trust ? m_value / mission_opinion.trust : 0;
            op.fear = mission_opinion.fear ? m_value / mission_opinion.fear : 0;
            op.value = mission_opinion.value ? m_value / mission_opinion.value : 0;
            op.anger = mission_opinion.anger ? m_value / mission_opinion.anger : 0;
            d.actor( true )->add_opinion( op );
        }
        if( d.actor( true )->turned_hostile() ) {
            d.actor( true )->make_angry();
            return talk_topic( "TALK_DONE" );
        }
    } else {
        for( const talk_effect_fun_t &effect : effects ) {
            effect( d );
        }
    }

    return next_topic;
}

void talk_effect_t::update_missions( dialogue &d )
{
    auto &ma = d.missions_assigned;
    ma.clear();
    if( d.has_beta ) {
        // Update the missions we can talk about (must only be current, non-complete ones)
        for( mission *&mission : d.actor( true )->assigned_missions() ) {
            if( mission->get_assigned_player_id() == d.actor( false )->getID() ) {
                ma.push_back( mission );
            }
        }
    }
}

talk_effect_t::talk_effect_t( const JsonObject &jo, const std::string &member_name,
                              std::string_view src )
{
    load_effect( jo, member_name, src );
    if( jo.has_object( "topic" ) ) {
        next_topic = load_inline_topic( jo.get_object( "topic" ), src );
    } else if( jo.has_string( "topic" ) ) {
        next_topic = talk_topic( jo.get_string( "topic" ) );
    }
}

void talk_effect_t::load_effect( const JsonObject &jo, const std::string &member_name,
                                 std::string_view src )
{
    if( jo.has_member( "opinion" ) ) {
        JsonValue jv = jo.get_member( "opinion" );
        // Same format as when saving a game (-:
        opinion.deserialize( jv );
    }
    if( jo.has_member( "mission_opinion" ) ) {
        JsonValue jv = jo.get_member( "mission_opinion" );
        // Same format as when saving a game (-:
        mission_opinion.deserialize( jv );
    }
    if( !jo.has_member( member_name ) ) {
        return;
    } else if( jo.has_string( member_name ) ) {
        const std::string type = jo.get_string( member_name );
        parse_string_effect( type, jo, src );
    } else if( jo.has_object( member_name ) ) {
        JsonObject sub_effect = jo.get_object( member_name );
        parse_sub_effect( sub_effect, src );
    } else if( jo.has_array( member_name ) ) {
        for( const JsonValue entry : jo.get_array( member_name ) ) {
            if( entry.test_string() ) {
                const std::string type = entry.get_string();
                parse_string_effect( type, jo, src );
            } else if( entry.test_object() ) {
                JsonObject sub_effect = entry.get_object();
                parse_sub_effect( sub_effect, src );
            } else {
                jo.throw_error_at( member_name, "invalid effect array syntax" );
            }
        }
    } else {
        jo.throw_error_at( member_name, "invalid effect syntax" );
    }
}

talk_response::talk_response()
{
    truefalse_condition = []( const_dialogue const & ) {
        return true;
    };
    mission_selected = nullptr;
    // Why aren't these null ids? Well, it turns out most responses give
    // empty ids, so things like the training code check for these empty ids
    // and when it's given a null id, it breaks
    // FIXME: Use null ids
    skill = skill_id();
    style = matype_id();
    proficiency = proficiency_id();
    dialogue_spell = spell_id();
}

talk_response::talk_response( const JsonObject &jo, std::string_view src )
{
    if( jo.has_member( "truefalsetext" ) ) {
        JsonObject truefalse_jo = jo.get_object( "truefalsetext" );
        read_condition( truefalse_jo, "condition", truefalse_condition, true );
        truefalse_jo.read( "true", truetext );
        truefalse_jo.read( "false", falsetext );
    } else {
        jo.read( "text", truetext );
        truefalse_condition = []( const_dialogue const & ) {
            return true;
        };
    }
    if( jo.has_member( "trial" ) ) {
        JsonObject trial_obj = jo.get_object( "trial" );
        trial = talk_trial( trial_obj );
    }
    if( jo.has_member( "success" ) ) {
        JsonObject success_obj = jo.get_object( "success" );
        success = talk_effect_t( success_obj, "effect", src );
    } else if( jo.has_string( "topic" ) ) {
        // This is for simple topic switching without a possible failure
        success.next_topic = talk_topic( jo.get_string( "topic" ) );
        success.load_effect( jo, "effect", src );
    } else if( jo.has_object( "topic" ) ) {
        success.next_topic = load_inline_topic( jo.get_object( "topic" ), src );
    }
    if( trial && !jo.has_member( "failure" ) ) {
        jo.throw_error( "the failure effect is mandatory if a talk_trial has been defined" );
    }
    if( jo.has_member( "failure" ) ) {
        JsonObject failure_obj = jo.get_object( "failure" );
        failure = talk_effect_t( failure_obj, "effect", src );
    }
    if( jo.has_member( "show_always" ) ) {
        show_always = jo.get_bool( "show_always" );
    }
    if( jo.has_member( "show_reason" ) ) {
        show_reason = jo.get_string( "show_reason" );
    }
    if( jo.has_member( "show_condition" ) ) {
        if( show_always ) {
            jo.throw_error( "show_always and show_condition cannot both be defined" );
        }
        read_condition( jo, "show_condition", show_condition, false );
    }
    // TODO: mission_selected
    // TODO: skill
    // TODO: style
}

json_talk_repeat_response::json_talk_repeat_response( const JsonObject &jo,
        std::string_view src )
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
        for( const std::string line : jo.get_array( "for_item" ) ) {
            for_item.emplace_back( line );
        }
    } else if( jo.has_string( "for_category" ) ) {
        for_category.emplace_back( jo.get_string( "for_category" ) );
    } else if( jo.has_array( "for_category" ) ) {
        for( const std::string line : jo.get_array( "for_category" ) ) {
            for_category.emplace_back( line );
        }
    } else {
        jo.throw_error( "Repeat response with no repeat information!" );
    }
    if( for_item.empty() && for_category.empty() ) {
        jo.throw_error( "Repeat response with empty repeat information!" );
    }
    if( jo.has_object( "response" ) ) {
        JsonObject response_obj = jo.get_object( "response" );
        response = json_talk_response( response_obj, src );
    } else {
        jo.throw_error( "Repeat response with no response!" );
    }
}

json_talk_response::json_talk_response( const JsonObject &jo, std::string_view src )
    : actual_response( jo, src )
{
    load_condition( jo, src );
    get_raw_debug_fields( jo, actual_response.debug_info );
    actual_response.condition = condition;
}

void json_talk_response::load_condition( const JsonObject &jo, std::string_view )
{
    has_condition_ = jo.has_member( "condition" );
    is_switch = jo.get_bool( "switch", false );
    is_default = jo.get_bool( "default", false );
    read_condition( jo, "condition", condition, true );

    optional( jo, true, "failure_explanation", failure_explanation );
    optional( jo, true, "failure_topic", failure_topic );
}

bool json_talk_response::test_condition( dialogue &d ) const
{
    if( condition ) {
        return condition( d );
    }
    //if condition doesn't exist, it evaluates to true if tested
    return true;
}

const talk_response &json_talk_response::get_actual_response() const
{
    return actual_response;
}

bool json_talk_response::gen_responses( dialogue &d, bool switch_done )
{
    bool condition_result = test_condition( d );

    if( !is_switch || !switch_done || d.debug_ignore_conditionals ) {
        if( condition_result || show_anyways( d ) || d.debug_ignore_conditionals ) {
            actual_response.ignore_conditionals = !condition_result && ( show_anyways( d ) ||
                                                  d.debug_ignore_conditionals );
            d.add_gen_response( actual_response, false, !!condition, condition_result );
            return is_switch && !is_default;
        } else if( !failure_explanation.empty() || !failure_topic.empty() ) {
            // build additional talk responses for failed options with an explanation if details are given
            talk_response tr = talk_response();
            tr.truetext = no_translation(
                              string_format( pgettext( "failure_explanation: actual_response", "*%s: %s" ),
                                             failure_explanation.translated(), actual_response.truetext.translated() ) );
            if( !failure_topic.empty() ) {
                // Default is TALK_NONE otherwise go to the failure topic provided
                tr.success.next_topic = talk_topic( failure_topic );
            }
            d.add_gen_response( tr, false, !!condition, condition_result );
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
            d.add_gen_response( result, !!condition, true, true );
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
            gender_map[subject] = d.actor( true )->get_grammatical_genders();
        } else if( subject == "u" ) {
            gender_map[subject] = d.actor( false )->get_grammatical_genders();
        } else {
            debugmsg( "Unsupported subject '%s' for grammatical gender in dialogue", subject );
        }
    }
    return gettext_gendered( gender_map, line );
}

dynamic_line_t dynamic_line_t::from_member( const JsonObject &jo,
        std::string_view member_name )
{
    if( jo.has_array( member_name ) ) {
        return dynamic_line_t( jo.get_array( member_name ) );
    } else if( jo.has_object( member_name ) ) {
        JsonObject dl = jo.get_object( member_name );
        if( dl.has_member( "str" ) ) {
            dl.allow_omitted_members();
            translation line;
            jo.read( member_name, line );
            return dynamic_line_t( line );
        } else {
            return dynamic_line_t( dl );
        }
    } else if( jo.has_string( member_name ) ) {
        translation line;
        jo.read( member_name, line );
        return dynamic_line_t( line );
    } else {
        return dynamic_line_t{};
    }
}

dynamic_line_t::dynamic_line_t( const translation &line )
{
    function = [line]( const dialogue & ) {
        return line.translated();
    };
}

dynamic_line_t::dynamic_line_t( const JsonObject &jo )
{
    if( jo.has_member( "concatenate" ) ) {
        std::vector<dynamic_line_t> lines;
        for( const JsonValue entry : jo.get_array( "concatenate" ) ) {
            if( entry.test_string() ) {
                translation line;
                entry.read( line );
                lines.emplace_back( line );
            } else if( entry.test_array() ) {
                lines.emplace_back( entry.get_array() );
            } else if( entry.test_object() ) {
                JsonObject dl = entry.get_object();
                if( dl.has_member( "str" ) ) {
                    dl.allow_omitted_members();
                    translation line;
                    entry.read( line );
                    lines.emplace_back( line );
                } else {
                    lines.emplace_back( dl );
                }
            } else {
                entry.throw_error( "invalid format: must be string, array or object" );
            }
        }
        function = [lines]( dialogue & d ) {
            std::string all_lines;
            for( const dynamic_line_t &line : lines ) {
                all_lines += line( d );
            }
            return all_lines;
        };
    } else if( jo.get_bool( "give_hint", false ) ) {
        function = [&]( const dialogue & ) {
            return get_hint();
        };
    } else if( jo.get_bool( "use_reason", false ) ) {
        function = [&]( const dialogue & d ) {
            std::string tmp = d.reason;
            d.reason.clear();
            return tmp;
        };
    } else if( jo.get_bool( "list_faction_camp_sites", false ) ) {
        function = [&]( const dialogue & ) {
            const auto &sites = recipe_group::get_recipes_by_id( "all_faction_base_types" );
            if( sites.empty() ) {
                return std::string( _( "I can't think of a single place I can build a camp." ) );
            }
            std::string tmp = "I can start a new camp as a ";
            tmp += enumerate_as_string( sites.begin(), sites.end(),
            []( const std::pair<recipe_id, translation> &site ) {
                return site.second.translated();
            },
            enumeration_conjunction::or_ );
            return tmp;
        };
    } else if( jo.has_string( "gendered_line" ) ) {
        std::string line;
        mandatory( jo, false, "gendered_line", line, text_style_check_reader() );
        if( !jo.has_array( "relevant_genders" ) ) {
            jo.throw_error(
                R"(dynamic line with "gendered_line" must also have "relevant_genders")" );
        }
        std::vector<std::string> relevant_genders;
        for( const std::string gender : jo.get_array( "relevant_genders" ) ) {
            relevant_genders.push_back( gender );
            if( gender != "npc" && gender != "u" ) {
                jo.throw_error( "Unexpected subject in relevant_genders; expected 'npc' or 'u'" );
            }
        }
        function = [line, relevant_genders]( const dialogue & d ) {
            return translate_gendered_line( line, relevant_genders, d );
        };
    } else {
        conditional_t dcondition;
        const dynamic_line_t yes = from_member( jo, "yes" );
        const dynamic_line_t no = from_member( jo, "no" );
        for( const std::string &sub_member : dialogue_data::simple_string_conds() ) {
            if( jo.has_bool( sub_member ) ) {
                // This also marks the member as visited.
                if( !jo.get_bool( sub_member ) ) {
                    jo.throw_error_at( sub_member, "value must be true" );
                }
                dcondition = conditional_t( sub_member );
                function = [dcondition, yes, no]( dialogue & d ) {
                    return ( dcondition( d ) ? yes : no )( d );
                };
                return;
            } else if( jo.has_member( sub_member ) ) {
                dcondition = conditional_t( sub_member );
                const dynamic_line_t yes_member = from_member( jo, sub_member );
                function = [dcondition, yes_member, no]( dialogue & d ) {
                    return ( dcondition( d ) ? yes_member : no )( d );
                };
                return;
            }
        }
        for( const std::string &sub_member : dialogue_data::complex_conds() ) {
            if( jo.has_member( sub_member ) ) {
                dcondition = conditional_t( jo );
                function = [dcondition, yes, no]( dialogue & d ) {
                    return ( dcondition( d ) ? yes : no )( d );
                };
                return;
            }
        }
        jo.throw_error( "dynamic line not supported" );
    }
}

dynamic_line_t::dynamic_line_t( const JsonArray &ja )
{
    std::vector<dynamic_line_t> lines;
    for( const JsonValue entry : ja ) {
        if( entry.test_string() ) {
            translation line;
            entry.read( line );
            lines.emplace_back( line );
        } else if( entry.test_array() ) {
            lines.emplace_back( entry.get_array() );
        } else if( entry.test_object() ) {
            JsonObject dl = entry.get_object();
            if( dl.has_member( "str" ) ) {
                dl.allow_omitted_members();
                translation line;
                entry.read( line );
                lines.emplace_back( line );
            } else {
                lines.emplace_back( dl );
            }
        } else {
            entry.throw_error( "invalid format: must be string, array or object" );
        }
    }
    function = [lines]( dialogue & d ) {
        const dynamic_line_t &line = random_entry_ref( lines );
        return line( d );
    };
}

json_dynamic_line_effect::json_dynamic_line_effect( const JsonObject &jo,
        const std::string &id, std::string_view src )
{
    std::function<bool( const_dialogue const & )> tmp_condition;
    read_condition( jo, "condition", tmp_condition, true );
    talk_effect_t tmp_effect = talk_effect_t( jo, "effect", src );
    get_raw_debug_fields( jo, debug_info );
    // if the topic has a sentinel, it means implicitly add a check for the sentinel value
    // and do not run the effects if it is set.  if it is not set, run the effects and
    // set the sentinel
    if( jo.has_string( "sentinel" ) ) {
        const std::string sentinel = jo.get_string( "sentinel" );
        const std::string varname = "sentinel_" + id + "_" + sentinel;
        condition = [varname, tmp_condition]( dialogue & d ) {
            return d.actor( false )->get_value( varname ).str() != "yes" && tmp_condition( d );
        };
        std::function<void( const dialogue &d )> function = [varname]( const dialogue & d ) {
            d.actor( false )->set_value( varname, "yes" );
        };
        tmp_effect.effects.emplace_back( function );
    } else {
        condition = tmp_condition;
    }
    effect = tmp_effect;
}

bool json_dynamic_line_effect::test_condition( dialogue &d ) const
{
    return condition( d );
}

void json_dynamic_line_effect::apply( dialogue &d ) const
{
    effect.apply( d );
}

void json_talk_topic::load( const JsonObject &jo, std::string_view src )
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
            JsonObject speaker_effect = jo.get_object( "speaker_effect" );
            speaker_effects.emplace_back( speaker_effect, id, src );
        } else if( jo.has_array( "speaker_effect" ) ) {
            for( JsonObject speaker_effect : jo.get_array( "speaker_effect" ) ) {
                speaker_effects.emplace_back( speaker_effect, id, src );
            }
        }
    }
    if( jo.has_member( "portrait_override" ) ) {
        portrait_override = character_portrait_id( jo.get_member( "portrait_override" ) );
    } else {
        // FIXME: Use real null IDs not std::optional juggling :(
        portrait_override = std::nullopt;
    }
    bool insert_above_bottom = false;
    if( jo.has_bool( "insert_before_standard_exits" ) ) {
        insert_above_bottom = jo.get_bool( "insert_before_standard_exits" );
    }
    if( !insert_above_bottom || responses.empty() ) {
        for( JsonObject response : jo.get_array( "responses" ) ) {
            responses.emplace_back( response, src );
        }
    } else {
        int dec_count = 0;
        if( !responses.empty() &&
            responses.back().get_actual_response().success.next_topic.id == "TALK_DONE" ) {
            dec_count = 1;
        }
        if( responses.size() >= 2 &&
            responses[ responses.size() - 2].get_actual_response().success.next_topic.id == "TALK_NONE" ) {
            dec_count = 2;
        }
        for( JsonObject response : jo.get_array( "responses" ) ) {
            responses.emplace( responses.end() - dec_count, response, src );
        }
    }
    if( jo.has_object( "repeat_responses" ) ) {
        repeat_responses.emplace_back( jo.get_object( "repeat_responses" ), src );
    } else if( jo.has_array( "repeat_responses" ) ) {
        for( JsonObject elem : jo.get_array( "repeat_responses" ) ) {
            repeat_responses.emplace_back( elem, src );
        }
    }
    if( responses.empty() ) {
        jo.throw_error_at( "responses", "no responses for talk topic defined" );
    }
    replace_built_in_responses = jo.get_bool( "replace_built_in_responses",
                                 replace_built_in_responses );
}

bool json_talk_topic::gen_responses( dialogue &d )
{
    d.responses.reserve( responses.size() ); // A wild guess, can actually be more or less

    bool switch_done = false;
    for( json_talk_response &r : responses ) {
        switch_done |= r.gen_responses( d, switch_done );
    }
    for( const json_talk_repeat_response &repeat : repeat_responses ) {
        talker *actor =  d.actor( repeat.is_npc );
        std::function<bool( const item & )> filter = return_true<item>;
        for( const itype_id &item_id : repeat.for_item ) {
            if( actor->charges_of( item_id ) > 0 || actor->has_amount( item_id, 1 ) ) {
                switch_done |= repeat.response.gen_repeat_response( d, item_id, switch_done );
            }
        }
        for( const item_category_id &category_id : repeat.for_category ) {
            const bool include_containers = repeat.include_containers;
            const auto items_with = actor->items_with( [category_id,
            include_containers]( const item & it ) {
                if( include_containers ) {
                    return it.get_category_of_contents().get_id() == category_id;
                }
                return it.type && it.type->category_force == category_id;
            } );
            for( item * const &it : items_with ) {
                switch_done |= repeat.response.gen_repeat_response( d, it->typeId(), switch_done );
            }
        }
    }

    return replace_built_in_responses;
}

cata::flat_set<std::string> json_talk_topic::get_directly_reachable_topics(
    bool only_unconditional ) const
{
    std::vector<std::string> result;

    auto add_reachable_for_response = [&]( const json_talk_response & json_response ) {
        const talk_response &response = json_response.get_actual_response();
        if( !only_unconditional || !json_response.has_condition() ) {
            result.push_back( response.success.next_topic.id );
            result.push_back( response.failure.next_topic.id );
        }
    };

    for( const json_talk_response &r : responses ) {
        add_reachable_for_response( r );
    }
    for( const json_talk_repeat_response &r : repeat_responses ) {
        add_reachable_for_response( r.response );
    }

    return cata::flat_set<std::string>( result.begin(), result.end() );
}

std::vector<json_talk_response> json_talk_topic::get_responses()
{
    return responses;
}

std::string json_talk_topic::get_dynamic_line( dialogue &d ) const
{
    return dynamic_line( d );
}

std::vector<json_dynamic_line_effect> json_talk_topic::get_speaker_effects() const
{
    return speaker_effects;
}

std::optional<character_portrait_id> json_talk_topic::get_portrait_override() const
{
    return portrait_override;
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

void load_talk_topic( const JsonObject &jo, std::string_view src )
{
    if( jo.has_array( "id" ) ) {
        for( auto &id : jo.get_string_array( "id" ) ) {
            json_talk_topics[id].load( jo, src );
        }
    } else {
        const std::string id = jo.get_string( "id" );
        json_talk_topics[id].load( jo, src );
    }
}

std::string npc::pick_talk_topic( const Character &/*u*/ )
{
    if( personality.aggression > 0 ) {
        if( op_of_u.fear * 2 < personality.bravery && personality.altruism < 0 ) {
            set_attitude( NPCATT_MUG );
            return chatbin.talk_mug ;
        }

        if( personality.aggression + personality.bravery - op_of_u.fear > 0 ) {
            return chatbin.talk_stranger_aggressive ;
        }
    }

    if( op_of_u.fear * 2 > personality.altruism + personality.bravery ) {
        return chatbin.talk_stranger_scared;
    }

    if( op_of_u.fear * 2 > personality.bravery + op_of_u.trust ) {
        return chatbin.talk_stranger_wary;
    }

    if( op_of_u.trust - op_of_u.fear +
        ( personality.bravery + personality.altruism ) / 2 > 0 ) {
        return chatbin.talk_stranger_friendly;
    }

    set_attitude( NPCATT_NULL );
    return chatbin.talk_stranger_neutral;
}

std::string npc::get_specified_talk_topic( std::string const &topic_id )
{
    static const dialogue_chatbin default_chatbin;
    std::vector<std::pair<std::string const &, std::string const &>> const talk_topics = {
        {default_chatbin.first_topic, chatbin.first_topic},
        {default_chatbin.talk_radio, chatbin.talk_radio},
        {default_chatbin.talk_leader, chatbin.talk_leader},
        {default_chatbin.talk_friend, chatbin.talk_friend},
        {default_chatbin.talk_stole_item, chatbin.talk_stole_item},
        {default_chatbin.talk_wake_up, chatbin.talk_wake_up},
        {default_chatbin.talk_mug, chatbin.talk_mug},
        {default_chatbin.talk_stranger_aggressive, chatbin.talk_stranger_aggressive},
        {default_chatbin.talk_stranger_scared, chatbin.talk_stranger_scared},
        {default_chatbin.talk_stranger_wary, chatbin.talk_stranger_wary},
        {default_chatbin.talk_stranger_friendly, chatbin.talk_stranger_friendly},
        {default_chatbin.talk_stranger_neutral, chatbin.talk_stranger_neutral},
        {default_chatbin.talk_friend_guard, chatbin.talk_friend_guard},
        {default_chatbin.talk_mission_inquire, chatbin.talk_mission_inquire},
        {default_chatbin.talk_mission_describe_urgent, chatbin.talk_mission_describe_urgent}
    };

    const auto iter = std::find_if( talk_topics.begin(), talk_topics.end(),
    [&topic_id]( const std::pair<std::string, std::string> &pair ) {
        return pair.first == topic_id;
    } );
    if( iter != talk_topics.end() ) {
        return iter->second;
    }

    return topic_id;
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

    auto_pickup::npc_settings &wlist = *rules.pickup_whitelist;
    const rule_state rule = wlist.check_item( to_match );
    if( rule == rule_state::WHITELISTED ) {
        return true;
    }

    if( rule == rule_state::BLACKLISTED ) {
        return false;
    }

    wlist.create_rule( to_match );
    return wlist.check_item( to_match ) == rule_state::WHITELISTED;
}

bool npc::item_whitelisted( const item &it )
{
    if( !has_item_whitelist() ) {
        return true;
    }

    const std::string to_match = it.tname( 1, false );
    return item_name_whitelisted( to_match );
}

const json_talk_topic *get_talk_topic( const std::string &id )
{
    auto it = json_talk_topics.find( id );
    if( it == json_talk_topics.end() ) {
        return nullptr;
    }
    return &it->second;
}

std::vector<std::string> get_all_talk_topic_ids()
{
    std::vector<std::string> dialogue_ids;
    dialogue_ids.reserve( json_talk_topics.size() );
    for( auto &elem : json_talk_topics ) {
        dialogue_ids.push_back( elem.first );
    }
    return dialogue_ids;
}
