#pragma once
#ifndef CATA_SRC_ITEM_CATEGORY_H
#define CATA_SRC_ITEM_CATEGORY_H

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "flat_set.h"
#include "translation.h"
#include "type_id.h"

class JsonObject;
class item;

// this is a helper struct with rules for picking a zone
struct zone_priority_data {
    bool was_loaded = false;
    zone_type_id id;
    bool filthy = false;
    cata::flat_set<flag_id> flags;

    void deserialize( const JsonObject &jo );
    void load( const JsonObject &jo );
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
        translation name_header_; // in inventory UI headers etc
        translation name_noun_; // in descriptive text
        /** Used to sort categories when displaying.  Lower values are shown first. */
        int sort_rank_ = 0;

        std::optional<zone_type_id> zone_;
        std::vector<zone_priority_data> zone_priority_;

    public:
        /** Unique ID of this category, used when loading from JSON. */
        item_category_id id;
        std::vector<std::pair<item_category_id, mod_id>> src;

        item_category() = default;
        /**
         * @param id @ref id_
         * @param name_header @ref name_header_
         * @param name_noun @ref name_noun_
         * @param sort_rank @ref sort_rank_
         */
        item_category( const item_category_id &id, const translation &name_header,
                       const translation &name_noun, const int sort_rank )
            : name_header_( name_header ), name_noun_( name_noun )
            , sort_rank_( sort_rank ), id( id ) {}
        item_category( const std::string &id, const translation &name_header,
                       const translation &name_noun, const int sort_rank )
            : name_header_( name_header ), name_noun_( name_noun )
            , sort_rank_( sort_rank ), id( item_category_id( id ) ) {}

        std::string name_header() const;
        std::string name_noun( int count ) const;
        item_category_id get_id() const;
        std::optional<zone_type_id> priority_zone( const item &it ) const;
        std::optional<zone_type_id> zone() const;
        int sort_rank() const;
        void set_spawn_rate( const float &rate ) const;
        float get_spawn_rate() const;

        /**
         * Comparison operators
         *
         * Used for sorting.  Will result in sorting by @ref sort_rank_, then by
         * @ref name_header_, then by @ref id.
         */
        /*@{*/
        bool operator<( const item_category &rhs ) const;
        bool operator==( const item_category &rhs ) const;
        bool operator!=( const item_category &rhs ) const;
        /*@}*/

        // generic_factory stuff
        bool was_loaded = false;

        static const std::vector<item_category> &get_all();
        static void load_item_cat( const JsonObject &jo, const std::string &src );
        static void reset();
        void load( const JsonObject &jo, std::string_view );
};

struct item_category_spawn_rates {
        static item_category_spawn_rates &get_item_category_spawn_rates() {
            static item_category_spawn_rates instance;
            return instance;
        }
        void set_spawn_rate( const item_category_id &id, const float &rate );
        float get_spawn_rate( const item_category_id &id );
    private:
        std::map<item_category_id, float> spawn_rates;
        item_category_spawn_rates() { }
};

#endif // CATA_SRC_ITEM_CATEGORY_H

