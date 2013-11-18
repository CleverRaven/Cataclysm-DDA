#ifndef DEPENDENCY_TREE_H
#define DEPENDENCY_TREE_H

#include <vector>
#include <map>
#include <stack>
#include <string>
#include <sstream>
#include <algorithm>

enum NODE_ERROR_TYPE
{
    DEPENDENCY,
    CYCLIC,
    OTHER
};

class dependency_node
{
public:
    std::vector<dependency_node*> parents, children;
    std::map<NODE_ERROR_TYPE, std::vector<std::string> > all_errors;
    std::string key;
    bool availability;

    // cyclic check variables
    int index, lowlink;
    bool on_stack;

    dependency_node();
    dependency_node(std::string _key);
    ~dependency_node();

    void add_parent(dependency_node *parent);
    void add_child (dependency_node *child);
    bool is_available();
    bool has_errors();
    std::map<NODE_ERROR_TYPE, std::vector<std::string > > errors();
    std::string s_errors();

    // Tree traversal
    // Upward towards head(s)
    std::vector<std::string> get_dependencies_as_strings();
    std::vector<dependency_node* > get_dependencies_as_nodes();
    // Downward towards leaf(ves)
    std::vector< std::string> get_dependents_as_strings();
    std::vector< dependency_node* > get_dependents_as_nodes();

    void inherit_errors();
    void check_cyclicity();
};

class dependency_tree
{
    public:
        /** Default constructor */
        dependency_tree();
        /** Default destructor */
        virtual ~dependency_tree();

        void init(std::map<std::string, std::vector<std::string> > key_dependency_map);

        void clear();

        // tree traversal
        // Upward by key
        std::vector<std::string > get_dependencies_of_X_as_strings(std::string key);
        std::vector<dependency_node* > get_dependencies_of_X_as_nodes(std::string key);
        // Downward by key
        std::vector< std::string > get_dependents_of_X_as_strings(std::string key);
        std::vector< dependency_node* > get_dependents_of_X_as_nodes(std::string key);

        bool is_available(std::string key);
        dependency_node *get_node(std::string key);

        std::map<std::string, dependency_node*> master_node_map;
    protected:
    private:
        // Don't need to be called directly. Only reason to call these are during initialization phase.
        void build_node_map(std::map<std::string, std::vector<std::string > > key_dependency_map);
        void build_connections(std::map<std::string, std::vector<std::string > > key_dependency_map);

        /*
        Cyclic Dependency checks using Tarjan's Strongly Connected Components algorithm
        Order is O(N+E) and linearly increases with number of nodes and number of connections between them.
        References:
            http://en.wikipedia.org/wiki/Tarjan's_strongly_connected_components_algorithm
            http://www.cosc.canterbury.ac.nz/tad.takaoka/alg/graphalg/sc.txt
        */
        void check_for_strongly_connected_components();
        void strong_connect(dependency_node *dnode);

        std::vector<std::vector<dependency_node* > > strongly_connected_components;
        std::stack<dependency_node*> connection_stack;
        std::vector<dependency_node*> nodes_on_stack;
        int open_index;

};

#endif // DEPENDENCY_TREE_H
