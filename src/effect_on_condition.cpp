#include "effect_on_condition.h"

#include <algorithm>
#include <cstddef>
#include <list>
#include <memory>
#include <unordered_map>
#include <ostream>
#include <queue>

#include "avatar.h"
#include "calendar.h"
#include "cata_utility.h"
#include "cata_variant.h"
#include "character.h"
#include "character_id.h"
#include "condition.h"
#include "creature.h"
#include "debug.h"
#include "event.h"
#include "flexbuffer_json.h"
#include "game.h"
#include "generic_factory.h"
#include "math_parser_diag_value.h"
#include "mod_tracker.h"
#include "npc.h"
#include "output.h"
#include "profession.h"
#include "scenario.h"
#include "string_formatter.h"
#include "talker.h"
#include "translations.h"
#include "type_id.h"

namespace io
{
    // *INDENT-OFF*
    template<>
    std::string enum_to_string<eoc_type>( eoc_type data )
    {
        switch ( data ) {
        case eoc_type::ACTIVATION: return "ACTIVATION";
        case eoc_type::RECURRING: return "RECURRING";
        case eoc_type::AVATAR_DEATH: return "AVATAR_DEATH";
        case eoc_type::NPC_DEATH: return "NPC_DEATH";
        case eoc_type::PREVENT_DEATH: return "PREVENT_DEATH";
        case eoc_type::EVENT: return "EVENT";
        case eoc_type::NUM_EOC_TYPES: break;
        }
        cata_fatal( "Invalid eoc_type" );
    }    
    // *INDENT-ON*
} // namespace io

namespace
{
generic_factory<effect_on_condition>
effect_on_condition_factory( "effect_on_condition" );
} // namespace

template<>
const effect_on_condition &effect_on_condition_id::obj() const
{
    return effect_on_condition_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<effect_on_condition>::is_valid() const
{
    return effect_on_condition_factory.is_valid( *this );
}

void effect_on_conditions::check_consistency()
{
}

void effect_on_condition::load( const JsonObject &jo, std::string_view src )
{
    mandatory( jo, was_loaded, "id", id );
    optional( jo, was_loaded, "eoc_type", type, eoc_type::NUM_EOC_TYPES );
    if( jo.has_member( "recurrence" ) ) {
        if( type != eoc_type::NUM_EOC_TYPES && type != eoc_type::RECURRING ) {
            jo.throw_error( "A recurring effect_on_condition must be of type RECURRING." );
        }
        type = eoc_type::RECURRING;
        recurrence = get_duration_or_var( jo, "recurrence", false );
    }
    if( type == eoc_type::NUM_EOC_TYPES ) {
        type = eoc_type::ACTIVATION;
    }

    if( jo.has_member( "deactivate_condition" ) ) {
        read_condition( jo, "deactivate_condition", deactivate_condition, false );
        has_deactivate_condition = true;
    }
    if( jo.has_member( "condition" ) ) {
        read_condition( jo, "condition", condition, false );
        has_condition = true;
    }
    true_effect.load_effect( jo, "effect", std::string( src ) );

    if( jo.has_member( "false_effect" ) ) {
        false_effect.load_effect( jo, "false_effect", std::string( src ) );
        has_false_effect = true;
    }

    optional( jo, was_loaded, "run_for_npcs", run_for_npcs, false );
    optional( jo, was_loaded, "global", global, false );
    if( !global && run_for_npcs ) {
        jo.throw_error( "run_for_npcs should only be true for global effect_on_conditions." );
    }

    if( type == eoc_type::EVENT ) {
        mandatory( jo, was_loaded, "required_event", required_event );
    }
}

effect_on_condition_id effect_on_conditions::load_inline_eoc( const JsonValue &jv,
        std::string_view src )
{
    if( jv.test_string() ) {
        return effect_on_condition_id( jv.get_string() );
    } else if( jv.test_object() ) {
        effect_on_condition inline_eoc;
        inline_eoc.load( jv.get_object(), src );
        mod_tracker::assign_src( inline_eoc, src );
        effect_on_condition_factory.insert( inline_eoc );
        return inline_eoc.id;
    } else {
        jv.throw_error( "effect_on_condition needs to be either a string or an effect_on_condition object." );
    }
}

static time_duration next_recurrence( const effect_on_condition_id &eoc, dialogue &d )
{
    return eoc->recurrence.evaluate( d );
}

void effect_on_conditions::load_new_character( Character &you )
{
    bool is_avatar = you.is_avatar();
    for( const effect_on_condition_id &eoc_id : get_scenario()->eoc() ) {
        effect_on_condition eoc = eoc_id.obj();
        if( is_avatar || eoc.run_for_npcs ) {
            queued_eoc new_eoc = queued_eoc{ eoc.id, calendar::turn_zero, {} };
            you.queued_effect_on_conditions.push( new_eoc );
        }
    }

    if( you.get_profession() ) {
        for( const effect_on_condition_id &eoc_id : you.get_profession()->get_eocs() ) {
            effect_on_condition eoc = eoc_id.obj();
            if( is_avatar || eoc.run_for_npcs ) {
                queued_eoc new_eoc = queued_eoc{ eoc.id, calendar::turn_zero, {} };
                you.queued_effect_on_conditions.push( new_eoc );
            }
        }
    }

    for( const effect_on_condition &eoc : effect_on_conditions::get_all() ) {
        if( eoc.type == eoc_type::RECURRING && ( ( is_avatar && eoc.global ) || !eoc.global ) ) {
            dialogue d( get_talker_for( you ), nullptr );
            queued_eoc new_eoc = queued_eoc{ eoc.id, calendar::turn + next_recurrence( eoc.id, d ), {} };
            if( eoc.global ) {
                g->queued_global_effect_on_conditions.push( new_eoc );
            } else {
                you.queued_effect_on_conditions.push( new_eoc );
            }
        }
    }

    effect_on_conditions::process_effect_on_conditions( you );
}

static void process_new_eocs( queued_eocs &eoc_queue,
                              std::vector<effect_on_condition_id> &eoc_vector,
                              std::map<effect_on_condition_id, bool> &new_eocs, bool global_queue )
{
    queued_eocs temp_queued_eocs;
    while( !eoc_queue.empty() ) {
        // Check if EoC is moved from global to local, or vice versa
        if( global_queue == eoc_queue.top().eoc->global ) {
            if( eoc_queue.top().eoc.is_valid() ) {
                temp_queued_eocs.push( eoc_queue.top() );
            }
            new_eocs[eoc_queue.top().eoc] = false;
        }
        eoc_queue.pop();
    }
    eoc_queue = std::move( temp_queued_eocs );
    for( auto eoc = eoc_vector.begin();
         eoc != eoc_vector.end(); ) {
        // Check if EoC is moved from global to local, or vice versa
        if( !eoc->is_valid() || global_queue != ( *eoc )->global ) {
            eoc = eoc_vector.erase( eoc );
        } else {
            new_eocs[*eoc] = false;
            eoc++;
        }
    }
}

void effect_on_conditions::load_existing_character( Character &you )
{
    bool is_avatar = you.is_avatar();
    std::map<effect_on_condition_id, bool> new_eocs;
    for( const effect_on_condition &eoc : effect_on_conditions::get_all() ) {
        if( eoc.type == eoc_type::RECURRING && ( is_avatar || !eoc.global ) ) {
            new_eocs[eoc.id] = true;
        }
    }
    process_new_eocs( you.queued_effect_on_conditions, you.inactive_effect_on_condition_vector,
                      new_eocs, false );
    if( is_avatar ) {
        process_new_eocs( g->queued_global_effect_on_conditions,
                          g->inactive_global_effect_on_condition_vector, new_eocs, true );
    }

    for( const std::pair<const effect_on_condition_id, bool> &eoc_pair : new_eocs ) {
        if( eoc_pair.second ) {
            dialogue d( get_talker_for( you ), nullptr );
            queue_effect_on_condition( next_recurrence( eoc_pair.first, d ),
                                       eoc_pair.first, you, {} );
        }
    }
}

void effect_on_conditions::queue_effect_on_condition( time_duration duration,
        effect_on_condition_id eoc, Character &you,
        global_variables::impl_t const &context )
{
    queued_eoc new_eoc = queued_eoc{ eoc, calendar::turn + duration, context };
    if( eoc->global ) {
        g->queued_global_effect_on_conditions.push( new_eoc );
    } else if( eoc->type == eoc_type::ACTIVATION || eoc->type == eoc_type::RECURRING ) {
        you.queued_effect_on_conditions.push( new_eoc );
    } else {
        debugmsg( "Invalid effect_on_condition and/or target.  EOC: %s", eoc->id.c_str() );
    }
}

static void process_eocs( queued_eocs &eoc_queue, std::vector<effect_on_condition_id> &eoc_vector,
                          dialogue &d )
{
    static std::vector<queued_eocs::storage_iter> eocs_to_queue;
    eocs_to_queue.clear();

    while( !eoc_queue.empty() &&
           eoc_queue.top().time <= calendar::turn ) {
        queued_eocs::storage_iter it = eoc_queue.queue.top();
        queued_eoc &top = *it;
        eoc_queue.queue.pop();

        dialogue nested_d{ d };
        for( const auto &val : top.context ) {
            nested_d.set_value( val.first, val.second );
        }
        bool activated = top.eoc->activate( nested_d );
        if( top.eoc->type == eoc_type::RECURRING ) {
            if( activated ) { // It worked so add it back
                it->time = calendar::turn + next_recurrence( top.eoc, d );
                eocs_to_queue.emplace_back( it );
            } else {
                if( !top.eoc->check_deactivate(
                        nested_d ) ) { // It failed but shouldn't be deactivated so add it back
                    it->time = calendar::turn + next_recurrence( top.eoc, d );
                    eocs_to_queue.emplace_back( it );
                } else { // It failed and should be deactivated for now
                    eoc_vector.push_back( top.eoc );
                    eoc_queue.list.erase( it );
                }
            }
        } else {
            eoc_queue.list.erase( it );
        }
    }
    for( queued_eocs::storage_iter &q_eoc : eocs_to_queue ) {
        eoc_queue.queue.emplace( q_eoc );
    }
}

void effect_on_conditions::process_effect_on_conditions( Character &you )
{
    dialogue d( get_talker_for( you ), nullptr );
    process_eocs( you.queued_effect_on_conditions, you.inactive_effect_on_condition_vector, d );
    //only handle global eocs on the avatars turn
    if( you.is_avatar() ) {
        process_eocs( g->queued_global_effect_on_conditions, g->inactive_global_effect_on_condition_vector,
                      d );
    }
}

static void process_reactivation( std::vector<effect_on_condition_id>
                                  &inactive_effect_on_condition_vector,
                                  queued_eocs &queued_effect_on_conditions, dialogue &d )
{
    std::vector<effect_on_condition_id> ids_to_reactivate;
    for( const effect_on_condition_id &eoc : inactive_effect_on_condition_vector ) {
        if( !eoc->check_deactivate( d ) ) {
            ids_to_reactivate.push_back( eoc );
        }
    }
    for( const effect_on_condition_id &eoc : ids_to_reactivate ) {
        queued_effect_on_conditions.push( queued_eoc{ eoc, calendar::turn + next_recurrence( eoc, d ), d.get_context() } );
        inactive_effect_on_condition_vector.erase( std::remove(
                    inactive_effect_on_condition_vector.begin(), inactive_effect_on_condition_vector.end(),
                    eoc ), inactive_effect_on_condition_vector.end() );
    }
}

void effect_on_conditions::process_reactivate( Character &you )
{
    dialogue d( get_talker_for( you ), nullptr );
    process_reactivation( you.inactive_effect_on_condition_vector, you.queued_effect_on_conditions, d );
}

void effect_on_conditions::process_reactivate()
{
    dialogue d( get_talker_for( get_avatar() ), nullptr );
    process_reactivation( g->inactive_global_effect_on_condition_vector,
                          g->queued_global_effect_on_conditions, d );
}

bool effect_on_condition::activate( dialogue &d, bool require_callstack_check ) const
{
    bool retval = false;
    if( require_callstack_check ) {
        d.amend_callstack( "EOC: " + id.str() );
        if( d.get_callstack().size() > 5000 ) {
            if( query_yn( string_format( _( "Possible infinite loop in EOC %s.  Stop execution?" ),
                                         id.str() ) ) ) {
                return false;
            }
        }
    }
    // each version needs a copy of the dialogue to pass down
    dialogue d_eoc( d );
    if( !has_condition || condition( d_eoc ) ) {
        true_effect.apply( d_eoc );
        retval = true;
    } else if( has_false_effect ) {
        false_effect.apply( d_eoc );
    }
    // This works because if global is true then this is recurring and thus should only ever be passed containing the player
    // Thus we just need to run the npcs.
    if( global && run_for_npcs ) {
        for( npc &guy : g->all_npcs() ) {
            dialogue d_npc( get_talker_for( guy ), nullptr, d.get_conditionals(), d.get_context() );
            if( !has_condition || condition( d_npc ) ) {
                true_effect.apply( d_npc );
            } else if( has_false_effect ) {
                false_effect.apply( d_npc );
            }
        }
    }
    return retval;
}

bool effect_on_condition::check_deactivate( const_dialogue const &d ) const
{
    if( !has_deactivate_condition || has_false_effect ) {
        return false;
    }
    return deactivate_condition( d );
}

bool effect_on_condition::test_condition( const_dialogue const &d ) const
{
    return !has_condition || condition( d );
}

void effect_on_condition::apply_true_effects( dialogue &d )  const
{
    true_effect.apply( d );
}

void effect_on_conditions::clear( Character &you )
{
    while( !you.queued_effect_on_conditions.empty() ) {
        you.queued_effect_on_conditions.pop();
    }
    you.inactive_effect_on_condition_vector.clear();
    while( !g->queued_global_effect_on_conditions.empty() ) {
        g->queued_global_effect_on_conditions.pop();
    }
    g->inactive_global_effect_on_condition_vector.clear();
}

void effect_on_conditions::write_eocs_to_file( Character &you )
{
    write_to_file( "eocs.output", [&you]( std::ostream & testfile ) {
        testfile << "Character Name: " + you.get_name() << std::endl;
        testfile << "id;timepoint;recurring" << std::endl;

        testfile << "queued eocs:" << std::endl;
        std::vector<queued_eoc> temp_queue;
        while( !you.queued_effect_on_conditions.empty() ) {
            temp_queue.push_back( you.queued_effect_on_conditions.top() );
            you.queued_effect_on_conditions.pop();
        }

        for( const queued_eoc &queue_entry : temp_queue ) {
            time_duration temp = queue_entry.time - calendar::turn;
            testfile << queue_entry.eoc.c_str() << ";" << to_string( temp ) << std::endl;
        }

        for( const queued_eoc &queued : temp_queue ) {
            you.queued_effect_on_conditions.push( queued );
        }

        testfile << "inactive eocs:" << std::endl;
        for( const effect_on_condition_id &eoc : you.inactive_effect_on_condition_vector ) {
            testfile << eoc.c_str() << std::endl;
        }

    }, "eocs test file" );
}

void effect_on_conditions::write_global_eocs_to_file( )
{
    write_to_file( "eocs.output", [&]( std::ostream & testfile ) {
        testfile << "global" << std::endl;
        testfile << "id;timepoint;recurring" << std::endl;

        testfile << "queued eocs:" << std::endl;
        std::vector<queued_eoc> temp_queue;
        while( !g->queued_global_effect_on_conditions.empty() ) {
            temp_queue.push_back( g->queued_global_effect_on_conditions.top() );
            g->queued_global_effect_on_conditions.pop();
        }

        for( const queued_eoc &queue_entry : temp_queue ) {
            time_duration temp = queue_entry.time - calendar::turn;
            testfile << queue_entry.eoc.c_str() << ";" << to_string( temp ) << std::endl;
        }

        for( const queued_eoc &queued : temp_queue ) {
            g->queued_global_effect_on_conditions.push( queued );
        }

        testfile << "inactive eocs:" << std::endl;
        for( const effect_on_condition_id &eoc : g->inactive_global_effect_on_condition_vector ) {
            testfile << eoc.c_str() << std::endl;
        }

    }, "eocs test file" );
}

void effect_on_conditions::prevent_death()
{
    avatar &player_character = get_avatar();
    dialogue d( get_talker_for( player_character ), nullptr );
    for( const effect_on_condition &eoc : effect_on_conditions::get_all() ) {
        if( eoc.type == eoc_type::PREVENT_DEATH ) {
            eoc.activate( d );
        }
        if( !player_character.is_dead_state() ) {
            player_character.clear_killer();
            break;
        }
    }
}

void effect_on_conditions::avatar_death()
{
    avatar &player_character = get_avatar();
    Creature *klr = player_character.get_killer();
    // Make sure the creature still exists in game
    klr = !klr ? klr : g->get_creature_if( [klr]( const Creature & c ) {
        return klr == &c;
    } );
    dialogue d( get_talker_for( get_avatar() ), klr == nullptr ? nullptr : get_talker_for( klr ) );
    for( const effect_on_condition &eoc : effect_on_conditions::get_all() ) {
        if( eoc.type == eoc_type::AVATAR_DEATH ) {
            eoc.activate( d );
        }
    }
}

void effect_on_condition::finalize()
{
}

void effect_on_conditions::finalize_all()
{
    effect_on_condition_factory.finalize();
    for( const effect_on_condition &eoc : effect_on_condition_factory.get_all() ) {
        const_cast<effect_on_condition &>( eoc ).finalize();
    }
}

void effect_on_condition::check() const
{
}

const std::vector<effect_on_condition> &effect_on_conditions::get_all()
{
    return effect_on_condition_factory.get_all();
}

void effect_on_conditions::reset()
{
    effect_on_condition_factory.reset();
}

void effect_on_conditions::load( const JsonObject &jo, const std::string &src )
{
    effect_on_condition_factory.load( jo, src );
}

void eoc_events::clear()
{
    has_cached = false;
    event_EOCs.clear();
}

void eoc_events::notify( const cata::event &e )
{
    notify( e, nullptr, nullptr );
}

void eoc_events::notify( const cata::event &e, std::unique_ptr<talker> alpha,
                         std::unique_ptr<talker> beta )
{
    if( !has_cached ) {

        // initialize all events to an empty vector
        for( event_type et = static_cast<event_type>( 0 ); et < event_type::num_event_types;
             et = static_cast<event_type>( static_cast<size_t>( et ) + 1 ) ) {

            event_EOCs[et] = std::vector<effect_on_condition>();
        }

        //create a cache for the specific types of EOC's so they aren't constantly all itterated through
        for( const effect_on_condition &eoc : effect_on_conditions::get_all() ) {
            if( eoc.type == eoc_type::EVENT ) {
                event_EOCs[eoc.required_event].emplace_back( eoc );
            }
        }

        has_cached = true;
    }

    for( const effect_on_condition &eoc : event_EOCs[e.type()] ) {
        if( !alpha ) {
            // try to assign a character for the EOC
            // TODO: refactor event_spec to take consistent inputs
            npc *alpha_talker = nullptr;
            const std::vector<std::string> potential_alphas = { "avatar_id", "character", "attacker", "killer", "npc" };
            for( const std::string &potential_key : potential_alphas ) {
                cata_variant cv = e.get_variant_or_void( potential_key );
                if( cv != cata_variant() ) {
                    character_id potential_id = cv.get<cata_variant_type::character_id>();
                    if( potential_id.is_valid() ) {
                        alpha_talker = g->find_npc( potential_id );
                        // if we find a successful entry exit early
                        break;
                    }
                }
            }
            if( alpha_talker ) {
                alpha = get_talker_for( alpha_talker );
            } else {
                alpha = get_talker_for( get_avatar() );
            }
        }
        dialogue d;
        global_variables::impl_t context;
        for( const auto &val : e.data() ) {
            context[val.first] = diag_value{ val.second };
        }

        // if we have an NPC to trigger this event for, do so,
        // otherwise fallback to having it effect the player
        d = dialogue( alpha->clone(), beta ? beta->clone() : nullptr, {}, context );

        eoc.activate( d );
    }
}
