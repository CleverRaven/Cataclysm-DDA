#include "behavior.h"

#include <assert.h>
#include <list>
#include <set>
#include <unordered_map>
#include <utility>

#include "behavior_oracle.h"
#include "behavior_strategy.h"
#include "generic_factory.h"
#include "debug.h"
#include "json.h"

using namespace behavior;

void node_t::set_strategy( const strategy_t *new_strategy )
{
    strategy = new_strategy;
}
void node_t::set_predicate( std::function<status_t ( const oracle_t * )> new_predicate )
{
    predicate = new_predicate;
}
void node_t::set_goal( const std::string &new_goal )
{
    _goal = new_goal;
}
void node_t::add_child( const node_t *new_child )
{
    children.push_back( new_child );
}

behavior_return node_t::tick( const oracle_t *subject ) const
{
    assert( predicate );
    if( children.empty() ) {
        return { predicate( subject ), this };
    } else {
        assert( strategy != nullptr );
        status_t result = predicate( subject );
        if( result == running ) {
            return strategy->evaluate( subject, children );
        } else {
            return { result, nullptr };
        }
    }
}

std::string node_t::goal() const
{
    return _goal;
}

std::string tree::tick( const oracle_t *subject )
{
    behavior_return result = root->tick( subject );
    active_node = result.result == running ? result.selection : nullptr;
    return goal();
}

std::string tree::goal() const
{
    return active_node == nullptr ? "idle" : active_node->goal();
}

void tree::add( const node_t *new_node )
{
    root = new_node;
}

// Now for the generic_factory definition

// This struct only exists to hold node data until finalization.
struct node_data {
    string_id<node_t> id;
    std::vector<std::string> children;
};

namespace
{
generic_factory<behavior::node_t> behavior_factory( "behavior" );
std::list<node_data> temp_node_data;
} // namespace

template<>
const node_t &string_id<node_t>::obj() const
{
    return behavior_factory.obj( *this );
}

void behavior::load_behavior( JsonObject &jo, const std::string &src )
{
    behavior_factory.load( jo, src );
}

node_t::node_t()
{
    predicate = &oracle_t::return_running;
}

void node_t::load( JsonObject &jo, const std::string & )
{
    // We don't initialize the node unless it has no children (opportunistic optimization).
    // Instead we initialize a parallel struct that holds the labels until finalization.
    if( jo.has_array( "children" ) ) {
        temp_node_data.emplace_back();
        node_data &new_data = temp_node_data.back();
        new_data.id = id;
        optional( jo, was_loaded, "children", new_data.children );
    }
    if( jo.has_string( "strategy" ) ) {
        std::unordered_map<std::string, const strategy_t *>::iterator new_strategy =
            behavior::strategy_map.find( jo.get_string( "strategy" ) );
        if( new_strategy != strategy_map.end() ) {
            strategy = new_strategy->second;
        } else {
            debugmsg( "While loading %s, failed to find strategy %s.",
                      id.str(), jo.get_string( "strategy" ) );
            jo.throw_error( "Invalid strategy in behavior." );
        }
    }
    if( jo.has_string( "predicate" ) ) {
        auto new_predicate = predicate_map.find( jo.get_string( "predicate" ) );
        if( new_predicate != predicate_map.end() ) {
            predicate = new_predicate->second;
        } else {
            debugmsg( "While loading %s, failed to find predicate %s.",
                      id.str(), jo.get_string( "predicate" ) );
            jo.throw_error( "Invalid strategy in behavior." );
        }
    }
    optional( jo, was_loaded, "goal", _goal );
}

void node_t::check() const
{
    // Invariants
    if( children.empty() ) {
        if( _goal.empty() ) {
            debugmsg( "Behavior %s must have either children or a goal.",
                      id.str() );
        }
    } else {
        if( strategy == nullptr ) {
            debugmsg( "Behavior %s has children but no strategy.", id.str() );
        }
        if( !_goal.empty() ) {
            debugmsg( "Behavior %s has both children and a goal.", id.str() );
        }
    }
}

void behavior::reset()
{
    temp_node_data.clear();
    behavior_factory.reset();
}

void behavior::finalize()
{
    for( const node_data &new_node : temp_node_data ) {
        for( const std::string child : new_node.children ) {
            const_cast<node_t &>( new_node.id.obj() ).
            add_child( &string_id<node_t>( child ).obj() );
        }
    }
    temp_node_data.clear();
}

void behavior::check_consistency()
{
    for( const node_t &checked_node : behavior_factory.get_all() ) {
        checked_node.check();
    }
}
