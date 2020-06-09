#pragma once
#ifndef CATA_SRC_BEHAVIOR_H
#define CATA_SRC_BEHAVIOR_H

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "string_id.h"

class JsonObject;

namespace behavior
{

class oracle_t;
class node_t;
class strategy_t;

enum class status_t : char { running, success, failure };
struct behavior_return {
    status_t result;
    const node_t *selection;
};

// The behavior tree is (at least initially) intended to decide on a goal for a given subject.
// Each time tick is called, it evaluates the tree to find the highest-priority behavior that is
// capable of making forward progress.
// The behavior tree performs a depth-first traversal until it reaches an unmet goal.
// When tick is invoked, it visits root -> node_t::tick -> strategy -> children -> node_t::tick
// At each level, the strategy determines the order in which to visit the current node's children.
// Once a leaf node is reached, it either returns success, indicating it's requirements are met,
// failure, indicating that it is incapable of satisfying it's requirements, or running, indicating
// that it is capable of addressing tis requirements, but that they aren't met yet.
// In practice, the tree is traversed until it reaches a node that returns running,
// meaning that it is capable of making progress if set as a goal.
// The arrangement of the tree and configuration of iteration strategies guarantees that it visits
// nodes in order of descending priority.
// A predicate can be injected into any node to direct iteration,
// including skipping entire subtrees.
class tree
{
    public:
        // Entry point, evaluates the tree and returns the selected goal.
        std::string tick( const oracle_t *subject );
        // Retrieves the most recently determined goal without re-evaluating the tree.
        std::string goal() const;
        // Set the root node of the tree.
        void add( const node_t *new_node );
    private:
        const node_t *root = nullptr;
        const node_t *active_node = nullptr;
};

class node_t
{
    public:
        node_t() = default;
        // Entry point for tree traversal.
        behavior_return tick( const oracle_t *subject ) const;
        std::string goal() const;

        // Interface to construct a node.
        void set_strategy( const strategy_t *new_strategy );
        void add_predicate( std::function < status_t ( const oracle_t *, const std::string & )>
                            new_predicate, const std::string &argument = "" );
        void set_goal( const std::string &new_goal );
        void add_child( const node_t *new_child );

        // Loading interface.
        void load( const JsonObject &jo, const std::string &src );
        void check() const;
        string_id<node_t> id;
        bool was_loaded = false;
    private:
        std::vector<const node_t *> children;
        const strategy_t *strategy = nullptr;
        using predicate_type = std::function<status_t( const oracle_t *, const std::string & )>;
        std::vector<std::pair<predicate_type, std::string>> conditions;
        // TODO: make into an ID?
        std::string _goal;
};

// Deserialization support.
void load_behavior( const JsonObject &jo, const std::string &src );

void reset();

void finalize();

void check_consistency();

} // namespace behavior

#endif // CATA_SRC_BEHAVIOR_H
