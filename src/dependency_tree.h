#pragma once
#ifndef DEPENDENCY_TREE_H
#define DEPENDENCY_TREE_H

#include "string_id.h"

#include <vector>
#include <map>
#include <stack>
#include <string>
#include <sstream>
#include <algorithm>
#include <memory>

struct MOD_INFORMATION;
using mod_id = string_id<MOD_INFORMATION>;

enum NODE_ERROR_TYPE {
    DEPENDENCY,
    CYCLIC,
    OTHER
};

class dependency_node
{
    public:
        std::vector<dependency_node *> parents, children;
        std::map<NODE_ERROR_TYPE, std::vector<std::string> > all_errors;
        mod_id key;
        bool availability;

        // cyclic check variables
        int index;
        int lowlink;
        bool on_stack;

        dependency_node();
        dependency_node( mod_id _key );

        void add_parent( dependency_node *parent );
        void add_child( dependency_node *child );
        bool is_available();
        bool has_errors();
        std::map<NODE_ERROR_TYPE, std::vector<std::string > > errors();
        std::string s_errors();

        // Tree traversal
        // Upward towards head(s)
        std::vector<mod_id> get_dependencies_as_strings();
        std::vector<dependency_node * > get_dependencies_as_nodes();
        // Downward towards leaf(ves)
        std::vector<mod_id> get_dependents_as_strings();
        std::vector< dependency_node * > get_dependents_as_nodes();

        void inherit_errors();
        void check_cyclicity();
};

class dependency_tree
{
    public:
        dependency_tree();

        void init( std::map<mod_id, std::vector<mod_id> > key_dependency_map );

        void clear();

        // tree traversal
        // Upward by key
        std::vector<mod_id > get_dependencies_of_X_as_strings( mod_id key );
        std::vector<dependency_node * > get_dependencies_of_X_as_nodes( mod_id key );
        // Downward by key
        std::vector< mod_id > get_dependents_of_X_as_strings( mod_id key );
        std::vector< dependency_node * > get_dependents_of_X_as_nodes( mod_id key );

        bool is_available( mod_id key );
        dependency_node *get_node( mod_id key );

        std::map<mod_id, dependency_node> master_node_map;
    protected:
    private:
        // Don't need to be called directly. Only reason to call these are during initialization phase.
        void build_node_map( std::map<mod_id, std::vector<mod_id > > key_dependency_map );
        void build_connections( std::map<mod_id, std::vector<mod_id > > key_dependency_map );

        /*
        Cyclic Dependency checks using Tarjan's Strongly Connected Components algorithm
        Order is O(N+E) and linearly increases with number of nodes and number of connections between them.
        References:
            http://en.wikipedia.org/wiki/Tarjan's_strongly_connected_components_algorithm
            http://www.cosc.canterbury.ac.nz/tad.takaoka/alg/graphalg/sc.txt
        */
        void check_for_strongly_connected_components();
        void strong_connect( dependency_node *dnode );

        std::vector<std::vector<dependency_node * > > strongly_connected_components;
        std::stack<dependency_node *> connection_stack;
        std::vector<dependency_node *> nodes_on_stack;
        int open_index;

};

#endif
