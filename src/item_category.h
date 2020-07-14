#pragma once
#ifndef CATA_SRC_ITEM_CATEGORY_H
#define CATA_SRC_ITEM_CATEGORY_H

#include <string>
#include <vector>

#include "flat_set.h"
#include "optional.h"
#include "translations.h"
#include "type_id.h"

class JsonIn;
class JsonObject;
class item;

// this is a helper struct with rules for picking a zone
struct zone_priority_data {
    bool was_loaded = false;
    zone_type_id id;
    bool filthy = false;
    cata::flat_set<std::string> flags;

    void deserialize( JsonIn &jsin );
    void load( JsonObject &jo );
};
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

        cata::optional<zone_type_id> zone_;
        std::vector<zone_priority_data> zone_priority_;

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
        cata::optional<zone_type_id> priority_zone( const item &it ) const;
        cata::optional<zone_type_id> zone() const;
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

        static void load_item_cat( const JsonObject &jo, const std::string &src );
        void load( const JsonObject &jo, const std::string & );
};

#endif // CATA_SRC_ITEM_CATEGORY_H

