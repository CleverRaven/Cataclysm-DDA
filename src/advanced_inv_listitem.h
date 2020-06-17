#pragma once
#ifndef CATA_SRC_ADVANCED_INV_LISTITEM_H
#define CATA_SRC_ADVANCED_INV_LISTITEM_H

#include <list>
#include <string>

#include "type_id.h"
#include "units.h"

// see item_factory.h
class item;
class item_category;

enum aim_location : char;

/**
 * Entry that is displayed in a adv. inv. pane. It can either contain a
 * single item or a category header or nothing (empty entry).
 * Most members are used only for sorting.
 */
class advanced_inv_listitem
{
    public:
        /**
         * Index of the item in the itemstack.
         */
        int idx = 0;
        /**
         * The location of the item, never AIM_ALL.
         */
        aim_location area;
        // the id of the item
        itype_id id;
        // The list of items, and empty when a header
        std::list<item *> items;
        /**
         * The displayed name of the item/the category header.
         */
        std::string name;
        /**
         * Name of the item (singular) without damage (or similar) prefix, used for sorting.
         */
        std::string name_without_prefix;
        /**
         * Whether auto pickup is enabled for this item (based on the name).
         */
        bool autopickup = false;
        /**
         * The stack count represented by this item, should be >= 1, should be 1
         * for anything counted by charges.
         */
        int stacks = 0;
        /**
         * The volume of all the items in this stack, used for sorting.
         */
        units::volume volume;
        /**
         * The weight of all the items in this stack, used for sorting.
         */
        units::mass weight = 0_gram;
        /**
         * The item category, or the category header.
         */
        const item_category *cat;
        /**
         * Is the item stored in a vehicle?
         */
        bool from_vehicle = false;
        /**
         * Whether this is a category header entry, which does *not* have a reference
         * to an item, only @ref cat is valid.
         */
        bool is_category_header() const;

        /** Returns true if this is an item entry */
        bool is_item_entry() const;
        /**
         * Create a category header entry.
         * @param cat The category reference, must not be null.
         */
        advanced_inv_listitem( const item_category *cat );
        /**
         * Creates an empty entry, both category and item pointer are null.
         */
        advanced_inv_listitem();
        /**
         * Create a normal item entry.
         * @param an_item The item pointer. Must not be null.
         * @param index The index
         * @param count The stack size
         * @param area The source area. Must not be AIM_ALL.
         * @param from_vehicle Is the item from a vehicle cargo space?
         */
        advanced_inv_listitem( item *an_item, int index, int count,
                               aim_location area, bool from_vehicle );
        /**
         * Create a normal item entry.
         * @param list The list of item pointers.
         * @param index The index
         * @param area The source area. Must not be AIM_ALL.
         * @param from_vehicle Is the item from a vehicle cargo space?
         */
        advanced_inv_listitem( const std::list<item *> &list, int index,
                               aim_location area, bool from_vehicle );
};
#endif // CATA_SRC_ADVANCED_INV_LISTITEM_H
