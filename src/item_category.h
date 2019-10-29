#pragma once
#ifndef ITEM_CATEGORY_H
#define ITEM_CATEGORY_H

#include <string>

#include "translations.h"
#include "type_id.h"

class JsonObject;

/**
 * Contains metadata for one category of items
 *
 * Every item belongs to a category (e.g. weapons, armor, food, etc).  This class
 * contains the info about one such category.  Actual categories are normally added
 * by class @ref Item_factory from definitions in the JSON data files.
 */
class item_category
{
    private:
        /** Name of category for displaying to the user */
        translation name_;
        /** Used to sort categories when displaying.  Lower values are shown first. */
        int sort_rank_ = 0;

    public:
        /** Unique ID of this category, used when loading from JSON. */
        item_category_id id;

        item_category() = default;
        /**
         * @param id @ref id_
         * @param name @ref name_
         * @param sort_rank @ref sort_rank_
         */
        item_category( const item_category_id &id, const translation &name, int sort_rank )
            : name_( name ), sort_rank_( sort_rank ), id( id ) {}
        item_category( const std::string &id, const translation &name, int sort_rank )
            : name_( name ), sort_rank_( sort_rank ), id( item_category_id( id ) ) {}

        std::string name() const;
        item_category_id get_id() const;
        int sort_rank() const;

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

        // generic_factory stuff
        bool was_loaded = false;

        static void load_item_cat( JsonObject &jo, const std::string &src );
        void load( JsonObject &jo, const std::string & );
};

#endif

