#include "effect_on_condition.h"

#include "avatar.h"
#include "cata_utility.h"
#include "character.h"
#include "condition.h"
#include "game.h"
#include "generic_factory.h"
#include "talker.h"
#include "type_id.h"

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

void effect_on_condition::load( const JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "id", id );
    activate_only = true;
    if( jo.has_member( "recurrence_min" ) || jo.has_member( "recurrence_max" ) ) {
        activate_only = false;
        mandatory( jo, was_loaded, "recurrence_min", recurrence_min );
        mandatory( jo, was_loaded, "recurrence_max", recurrence_max );
        if( recurrence_max < recurrence_min ) {
            jo.throw_error( "recurrence_max cannot be smaller than recurrence_min." );
        }
    }
    if( jo.has_member( "deactivate_condition" ) ) {
        read_condition<dialogue>( jo, "deactivate_condition", deactivate_condition, false );
        has_deactivate_condition = true;
    }
    if( jo.has_member( "condition" ) ) {
        read_condition<dialogue>( jo, "condition", condition, false );
        has_condition = true;
    }
    true_effect.load_effect( jo, "effect" );

    if( jo.has_member( "false_effect" ) ) {
        false_effect.load_effect( jo, "false_effect" );
        has_false_effect = true;
    }
}

effect_on_condition_id effect_on_conditions::load_inline_eoc( const JsonValue &jv,
        const std::string &src )
{
    if( jv.test_string() ) {
        return effect_on_condition_id( jv.get_string() );
    } else if( jv.test_object() ) {
        effect_on_condition inline_eoc;
        inline_eoc.load( jv.get_object(), src );
        effect_on_condition_factory.insert( inline_eoc );
        return inline_eoc.id;
    } else {
        jv.throw_error( "effect_on_condition needs to be either a string or an effect_on_condition object." );
    }
}

static time_duration next_recurrence( const effect_on_condition_id &eoc )
{
    return rng( eoc->recurrence_min, eoc->recurrence_max );
}

void effect_on_conditions::load_new_character()
{
    for( const effect_on_condition &eoc : effect_on_conditions::get_all() ) {
        if( !eoc.activate_only ) {
            queued_eoc new_eoc = queued_eoc{ eoc.id, true, calendar::turn + next_recurrence( eoc.id ) };
            g->queued_effect_on_conditions.push( new_eoc );
        }
    }
}

void effect_on_conditions::queue_effect_on_condition( time_duration duration,
        effect_on_condition_id eoc )
{
    queued_eoc new_eoc = queued_eoc{ eoc, false, calendar::turn + duration };
    g->queued_effect_on_conditions.push( new_eoc );
}

void effect_on_conditions::process_effect_on_conditions()
{
    dialogue d;
    standard_npc default_npc( "Default" );
    d.alpha = get_talker_for( get_avatar() );
    d.beta = get_talker_for( default_npc );
    std::vector<queued_eoc> eocs_to_queue;
    while( !g->queued_effect_on_conditions.empty() &&
           g->queued_effect_on_conditions.top().time <= calendar::turn ) {
        queued_eoc top = g->queued_effect_on_conditions.top();
        bool activated = top.eoc->activate( d );
        if( top.recurring ) {
            if( activated ) { // It worked so add it back
                queued_eoc new_eoc = queued_eoc{ top.eoc, true, calendar::turn + next_recurrence( top.eoc ) };
                eocs_to_queue.push_back( new_eoc );
            } else {
                if( !top.eoc->check_deactivate() ) { // It failed but shouldn't be deactivated so add it back
                    queued_eoc new_eoc = queued_eoc{ top.eoc, true, calendar::turn + next_recurrence( top.eoc ) };
                    eocs_to_queue.push_back( new_eoc );
                } else { // It failed and should be deactivated for now
                    g->inactive_effect_on_condition_vector.push_back( top.eoc );
                }
            }
        }
        g->queued_effect_on_conditions.pop();
    }
    for( const queued_eoc &q_eoc : eocs_to_queue ) {
        g->queued_effect_on_conditions.push( q_eoc );
    }
}

void effect_on_conditions::process_reactivate()
{
    dialogue d;
    standard_npc default_npc( "Default" );
    d.alpha = get_talker_for( get_avatar() );
    d.beta = get_talker_for( default_npc );

    std::vector<effect_on_condition_id> ids_to_reactivate;
    for( const effect_on_condition_id &eoc : g->inactive_effect_on_condition_vector ) {
        if( !eoc->check_deactivate() ) {
            ids_to_reactivate.push_back( eoc );
        }
    }
    for( const effect_on_condition_id &eoc : ids_to_reactivate ) {
        g->queued_effect_on_conditions.push( queued_eoc{ eoc, true, calendar::turn + next_recurrence( eoc ) } );
        g->inactive_effect_on_condition_vector.erase( std::remove(
                    g->inactive_effect_on_condition_vector.begin(), g->inactive_effect_on_condition_vector.end(), eoc ),
                g->inactive_effect_on_condition_vector.end() );
    }
}

bool effect_on_condition::activate( dialogue &d ) const
{
    if( !has_condition || condition( d ) ) {
        true_effect.apply( d );
        return true;
    }

    if( has_false_effect ) {
        false_effect.apply( d );
    }
    return false;
}

bool effect_on_condition::check_deactivate() const
{
    if( !has_deactivate_condition || has_false_effect ) {
        return false;
    }
    dialogue d;
    standard_npc default_npc( "Default" );
    d.alpha = get_talker_for( get_avatar() );
    d.beta = get_talker_for( default_npc );
    return deactivate_condition( d );
}

void effect_on_conditions::clear()
{
    while( !g->queued_effect_on_conditions.empty() ) {
        g->queued_effect_on_conditions.pop();
    }
    g->inactive_effect_on_condition_vector.clear();
}

void effect_on_conditions::write_eocs_to_file()
{
    write_to_file( "eocs.output", [&]( std::ostream & testfile ) {
        testfile << "id;timepoint;recurring" << std::endl;

        testfile << "queued eocs:" << std::endl;
        std::vector<queued_eoc> temp_queue;
        while( !g->queued_effect_on_conditions.empty() ) {
            temp_queue.push_back( g->queued_effect_on_conditions.top() );
            g->queued_effect_on_conditions.pop();
        }

        for( const auto &queue_entry : temp_queue ) {
            time_duration temp = queue_entry.time - calendar::turn;
            testfile << queue_entry.eoc.c_str() << ";" << to_string( temp ) << ";" <<
                     ( queue_entry.recurring ? "recur" : "non" ) << std::endl ;
        }

        for( const auto &queued : temp_queue ) {
            g->queued_effect_on_conditions.push( queued );
        }

        testfile << "inactive eocs:" << std::endl;
        for( const effect_on_condition_id &eoc : g->inactive_effect_on_condition_vector ) {
            testfile << eoc.c_str() << std::endl;
        }

    }, "eocs test file" );
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
