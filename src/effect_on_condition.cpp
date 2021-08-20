#include "effect_on_condition.h"

#include "avatar.h"
#include "cata_utility.h"
#include "character.h"
#include "condition.h"
#include "game.h"
#include "generic_factory.h"
#include "scenario.h"
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

    optional( jo, was_loaded, "scenario_specific", scenario_specific );
    optional( jo, was_loaded, "run_for_npcs", run_for_npcs, false );
    optional( jo, was_loaded, "global", global, false );
    if( activate_only && ( global || run_for_npcs ) ) {
        jo.throw_error( "run_for_npcs and global should only be true for recurring effect_on_conditions." );
    } else if( global && run_for_npcs ) {
        jo.throw_error( "An effect_on_condition can be either run_for_npcs or global but not both." );
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

void effect_on_conditions::load_new_character( Character &you )
{
    bool is_avatar = you.is_avatar();
    for( const effect_on_condition_id &eoc_id : get_scenario()->eoc() ) {
        effect_on_condition eoc = eoc_id.obj();
        if( eoc.scenario_specific ) {
            queued_eoc new_eoc = queued_eoc{ eoc.id, true, calendar::turn + next_recurrence( eoc.id ) };
            you.queued_effect_on_conditions.push( new_eoc );
        }
    }
    effect_on_conditions::process_effect_on_conditions( you );
    effect_on_conditions::clear( you );

    for( const effect_on_condition &eoc : effect_on_conditions::get_all() ) {
        if( !eoc.activate_only && !eoc.scenario_specific && ( is_avatar || eoc.run_for_npcs ) ) {
            queued_eoc new_eoc = queued_eoc{ eoc.id, true, calendar::turn + next_recurrence( eoc.id ) };
            you.queued_effect_on_conditions.push( new_eoc );
        }
    }

    effect_on_conditions::process_effect_on_conditions( you );
}

void effect_on_conditions::load_existing_character( Character &you )
{
    bool is_avatar = you.is_avatar();
    std::map<effect_on_condition_id, bool> new_eocs;
    for( const effect_on_condition &eoc : effect_on_conditions::get_all() ) {
        if( !eoc.activate_only && !eoc.scenario_specific && ( is_avatar || eoc.run_for_npcs ) ) {
            new_eocs[eoc.id] = true;
        }
    }

    std::priority_queue<queued_eoc, std::vector<queued_eoc>, eoc_compare>
    temp_queued_effect_on_conditions;
    while( !you.queued_effect_on_conditions.empty() ) {
        if( you.queued_effect_on_conditions.top().eoc.is_valid() ) {
            temp_queued_effect_on_conditions.push( you.queued_effect_on_conditions.top() );
        }
        new_eocs[you.queued_effect_on_conditions.top().eoc] = false;
        you.queued_effect_on_conditions.pop();
    }
    you.queued_effect_on_conditions = temp_queued_effect_on_conditions;
    for( auto eoc = you.inactive_effect_on_condition_vector.begin();
         eoc != you.inactive_effect_on_condition_vector.end(); eoc++ ) {
        if( !eoc->is_valid() ) {
            eoc = you.inactive_effect_on_condition_vector.erase( eoc );
        } else {
            new_eocs[*eoc] = false;
        }
    }
    for( const std::pair<const effect_on_condition_id, bool> &eoc_pair : new_eocs ) {
        if( eoc_pair.second ) {
            queue_effect_on_condition( next_recurrence( eoc_pair.first ), eoc_pair.first );
        }
    }
}

void effect_on_conditions::queue_effect_on_condition( time_duration duration,
        effect_on_condition_id eoc )
{
    queued_eoc new_eoc = queued_eoc{ eoc, false, calendar::turn + duration };
    get_player_character().queued_effect_on_conditions.push( new_eoc );
}

void effect_on_conditions::process_effect_on_conditions( Character &you )
{
    dialogue d( get_talker_for( you ), nullptr );
    std::vector<queued_eoc> eocs_to_queue;
    while( !you.queued_effect_on_conditions.empty() &&
           ( you.queued_effect_on_conditions.top().eoc.obj().scenario_specific ||
             you.queued_effect_on_conditions.top().time <= calendar::turn ) ) {
        queued_eoc top = you.queued_effect_on_conditions.top();
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
                    you.inactive_effect_on_condition_vector.push_back( top.eoc );
                }
            }
        }
        you.queued_effect_on_conditions.pop();
    }
    for( const queued_eoc &q_eoc : eocs_to_queue ) {
        you.queued_effect_on_conditions.push( q_eoc );
    }
}

void effect_on_conditions::process_reactivate( Character &you )
{
    std::vector<effect_on_condition_id> ids_to_reactivate;
    for( const effect_on_condition_id &eoc : you.inactive_effect_on_condition_vector ) {
        if( !eoc->check_deactivate() ) {
            ids_to_reactivate.push_back( eoc );
        }
    }
    for( const effect_on_condition_id &eoc : ids_to_reactivate ) {
        you.queued_effect_on_conditions.push( queued_eoc{ eoc, true, calendar::turn + next_recurrence( eoc ) } );
        you.inactive_effect_on_condition_vector.erase( std::remove(
                    you.inactive_effect_on_condition_vector.begin(), you.inactive_effect_on_condition_vector.end(),
                    eoc ), you.inactive_effect_on_condition_vector.end() );
    }
}

bool effect_on_condition::activate( dialogue &d ) const
{
    bool retval = false;
    if( !has_condition || condition( d ) ) {
        true_effect.apply( d );
        retval = true;
    } else if( has_false_effect ) {
        false_effect.apply( d );
    }
    // This works because if global is true then this is recurring and thus should only ever be passed containing the player
    // Thus we just need to run the npcs.
    if( global ) {
        for( npc &guy : g->all_npcs() ) {
            dialogue d_npc( get_talker_for( guy ), nullptr );
            if( !has_condition || condition( d_npc ) ) {
                true_effect.apply( d_npc );
            } else if( has_false_effect ) {
                false_effect.apply( d_npc );
            }
        }
    }
    return retval;
}

bool effect_on_condition::check_deactivate() const
{
    if( !has_deactivate_condition || has_false_effect ) {
        return false;
    }
    dialogue d( get_talker_for( get_avatar() ), nullptr );
    return deactivate_condition( d );
}

void effect_on_conditions::clear( Character &you )
{
    while( !you.queued_effect_on_conditions.empty() ) {
        you.queued_effect_on_conditions.pop();
    }
    you.inactive_effect_on_condition_vector.clear();
}

void effect_on_conditions::write_eocs_to_file( Character &you )
{
    write_to_file( "eocs.output", [&you]( std::ostream & testfile ) {
        testfile << "id;timepoint;recurring" << std::endl;

        testfile << "queued eocs:" << std::endl;
        std::vector<queued_eoc> temp_queue;
        while( !you.queued_effect_on_conditions.empty() ) {
            temp_queue.push_back( you.queued_effect_on_conditions.top() );
            you.queued_effect_on_conditions.pop();
        }

        for( const auto &queue_entry : temp_queue ) {
            time_duration temp = queue_entry.time - calendar::turn;
            testfile << queue_entry.eoc.c_str() << ";" << to_string( temp ) << ";" <<
                     ( queue_entry.recurring ? "recur" : "non" ) << std::endl ;
        }

        for( const auto &queued : temp_queue ) {
            you.queued_effect_on_conditions.push( queued );
        }

        testfile << "inactive eocs:" << std::endl;
        for( const effect_on_condition_id &eoc : you.inactive_effect_on_condition_vector ) {
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
