#pragma once
#ifndef ITEM_CATEGORY_H
#define ITEM_CATEGORY_H

#include <string>

/**
 * Contains metadata for one category of items
 *
 * Every item belongs to a category (e.g. weapons, armor, food, etc).  This class
 * contains the info about one such category.  Actual categories are normally added
 * by class @ref Item_factory from definitions in the JSON data files.
 */
class item_category
{
    public:
        /** Unique ID of this category, used when loading from JSON. */
        std::string id;
        /** Name of category for displaying to the user (localized) */
        std::string name;
        /** Used to sort categories when displaying.  Lower values are shown first. */
        int sort_rank = 0;

        item_category() = default;
        /**
         * @param id @ref id
         * @param name @ref name
         * @param sort_rank @ref sort_rank
         */
        item_category( const std::string &id, const std::string &name, int sort_rank ) : id( id ),
            name( name ), sort_rank( sort_rank ) { }

        /**
         * Comparison operators
         *
         * Used for sorting.  Will result in sorting by @ref sort_rank, then by @ref name, then by @ref id.
         */
        /*@{*/
        bool operator<( const item_category &rhs ) const;
        bool operator==( const item_category &rhs ) const;
        bool operator!=( const item_category &rhs ) const;
        /*@}*/
};

#endif

