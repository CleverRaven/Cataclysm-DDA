#include "effect_on_condition.h"

#include "avatar.h"
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

void effect_on_condition::load( const JsonObject &jo, const std::string &src )
{
    bool activate_only = false;
    optional( jo, was_loaded, "activate_only", activate_only, false );
    optional( jo, was_loaded, "once_every", once_every, 6000_seconds );
    if( jo.has_member( "deactivate_condition" ) ) {
        read_condition<dialogue>( jo, "deactivate_condition", deactivate_condition, false );
        has_deactivate_condition = true;
    }
    jdle = json_dynamic_line_effect( jo, src );
    if( !activate_only ) {
        g->active_effect_on_condition_vector.push_back( id );
    }
}

void effect_on_conditions::queue_effect_on_condition( time_duration duration,
        effect_on_condition_id eoc )
{
    g->queued_effect_on_conditions.push( std::make_pair( calendar::turn + duration, eoc ) );
}

void effect_on_conditions::process_effect_on_conditions()
{
    dialogue d;
    standard_npc default_npc( "Default" );
    d.alpha = get_talker_for( get_avatar() );
    d.beta = get_talker_for( default_npc );
    while( !g->queued_effect_on_conditions.empty() &&
           g->queued_effect_on_conditions.top().first <= calendar::turn ) {
        g->queued_effect_on_conditions.top().second->activate();
        g->queued_effect_on_conditions.pop();
    }

    std::vector<effect_on_condition_id> ids_to_remove;
    for( const effect_on_condition_id &eoc : g->active_effect_on_condition_vector ) {
        if( calendar::once_every( eoc->once_every ) ) {
            bool activated = eoc->activate();
            if( !activated && eoc->check_deactivate() ) {
                ids_to_remove.push_back( eoc );
            }
        }
    }
    for( const effect_on_condition_id &eoc : ids_to_remove ) {
        g->inactive_effect_on_condition_vector.push_back( eoc );
        g->active_effect_on_condition_vector.erase( std::remove(
                    g->active_effect_on_condition_vector.begin(), g->active_effect_on_condition_vector.end(), eoc ),
                g->active_effect_on_condition_vector.end() );
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
        g->active_effect_on_condition_vector.push_back( eoc );
        g->inactive_effect_on_condition_vector.erase( std::remove(
                    g->active_effect_on_condition_vector.begin(), g->active_effect_on_condition_vector.end(), eoc ),
                g->active_effect_on_condition_vector.end() );
    }
}

bool effect_on_condition::activate() const
{
    dialogue d;
    standard_npc default_npc( "Default" );
    d.alpha = get_talker_for( get_avatar() );
    d.beta = get_talker_for( default_npc );
    if( jdle.test_condition( d ) ) {
        jdle.apply( d );
        return true;
    }
    return false;
}

bool effect_on_condition::check_deactivate() const
{
    if( !has_deactivate_condition ) {
        return false;
    }
    dialogue d;
    standard_npc default_npc( "Default" );
    d.alpha = get_talker_for( get_avatar() );
    d.beta = get_talker_for( default_npc );
    return deactivate_condition( d );
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
