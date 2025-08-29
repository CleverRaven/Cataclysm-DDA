#pragma once
#ifndef CATA_SRC_MAP_ITEM_STACK_H
#define CATA_SRC_MAP_ITEM_STACK_H

#include <string>
#include <vector>

#include "coordinates.h"
#include "point.h"

class item;

class map_item_stack
{
    private:
        class item_group
        {
            public:
                tripoint_rel_ms pos;
                int count;
                const item *it;

                //only expected to be used for things like lists and vectors
                item_group();
                item_group( const tripoint_rel_ms &p, int arg_count, const item *itm );
        };
    public:
        const item *example; //an example item for showing stats, etc.
        std::vector<item_group> vIG;
        int totalcount;

        //only expected to be used for things like lists and vectors
        map_item_stack();
        map_item_stack( const item *it, const tripoint_rel_ms &pos );

        // This adds to an existing item group if the last current
        // item group is the same position and otherwise creates and
        // adds to a new item group. Note that it does not search
        // through all older item groups for a match.
        void add_at_pos( const item *it, const tripoint_rel_ms &pos );

        static bool compare_item_names( const map_item_stack &lhs,
                                        const map_item_stack &rhs );
        static bool map_item_stack_sort_category_distance( const map_item_stack &lhs,
                const map_item_stack &rhs );
        static bool map_item_stack_sort_category_name( const map_item_stack &lhs,
                const map_item_stack &rhs );
        static bool map_item_stack_sort_name( const map_item_stack &lhs, const map_item_stack &rhs );
};

std::vector<map_item_stack> filter_item_stacks( const std::vector<map_item_stack> &stack,
        const std::string &filter );
int list_filter_high_priority( std::vector<map_item_stack> &stack, const std::string &priorities );
int list_filter_low_priority( std::vector<map_item_stack> &stack, int start,
                              const std::string &priorities );

#endif // CATA_SRC_MAP_ITEM_STACK_H
