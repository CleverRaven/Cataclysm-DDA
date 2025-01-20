#pragma once
#ifndef CATA_SRC_MAP_ENTITY_STACK_H
#define CATA_SRC_MAP_ENTITY_STACK_H

#include "coordinates.h"
#include "point.h"

template<typename T>
class map_entity_stack
{
    private:
        class entity_group
        {
            public:
                tripoint_rel_ms pos;
                int count;
                const T *entity;
                std::optional<std::string> cached_name;
                std::optional<std::string> cached_distance_string;

                //only expected to be used for things like lists and vectors
                entity_group() : count( 0 ), entity( nullptr ) {};
                entity_group( const tripoint_rel_ms &p, int arg_count, const T *entity ) :
                    pos( p ), count( arg_count ), entity( entity ) {};
        };

        int selected_index = 0;
        void make_name_cache();
        void make_distance_string_cache();
    public:
        std::vector<entity_group> entities;
        int totalcount;

        const T *get_selected_entity() const;
        std::optional<tripoint_rel_ms> get_selected_pos() const;
        int get_selected_count() const;
        std::string get_selected_name();
        std::string get_selected_distance_string();
        int get_selected_index() const;

        void select_next();
        void select_prev();

        //only expected to be used for things like lists and vectors
        map_entity_stack();
        map_entity_stack( const T *entity, const tripoint_rel_ms &pos, int count = 1 );

        // This adds to an existing entity group if the last current
        // entity group is the same position and otherwise creates and
        // adds to a new entity group. Note that it does not search
        // through all older entity groups for a match.
        void add_at_pos( const T *entity, const tripoint_rel_ms &pos, int count = 1 );

        bool compare( const map_entity_stack<T> &rhs, bool use_category = false ) const;
        const std::string get_category() const;
};

template<typename T>
int list_filter_high_priority( std::vector<map_entity_stack<T>*> &stack,
                               const std::string &priorities );
template<typename T>
int list_filter_low_priority( std::vector<map_entity_stack<T>*> &stack, int start,
                              const std::string &priorities );

#endif // CATA_SRC_MAP_ENTITY_STACK_H
