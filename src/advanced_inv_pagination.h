#pragma once
#ifndef CATA_SRC_ADVANCED_INV_PAGINATION_H
#define CATA_SRC_ADVANCED_INV_PAGINATION_H

class advanced_inventory_pane;
class item_category;

/**
 * This class determines the page and line at which an item appears in the AIM.
 */
class advanced_inventory_pagination
{
    private:
        const int linesPerPage;
        const advanced_inventory_pane &pane;
        int lines_needed( int index );
    public:
        int line;
        int page;
        const item_category *last_category;
        advanced_inventory_pagination( int linesPerPage, const advanced_inventory_pane &pane )
            : linesPerPage( linesPerPage ), pane( pane ), line( 0 ), page( 0 ), last_category( nullptr ) { }

        /// Reset pagination state to the start of the current page, so it can be printed.
        void reset_page();

        /// Returns true if printing an item with the category requires a category header.
        bool new_category( const item_category *cat ) const;

        /// Step the pagination state forward for the item with this index.
        /// Returns true if printing the item required starting a new page.
        bool step( int index );
};
#endif // CATA_SRC_ADVANCED_INV_PAGINATION_H
