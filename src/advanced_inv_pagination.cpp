#include "advanced_inv_pagination.h"

#include <memory>
#include <vector>

#include "advanced_inv_pane.h"

class item_category;

void advanced_inventory_pagination::reset_page()
{
    line = 0;
    page = 0;
    last_category = nullptr;
}

bool advanced_inventory_pagination::new_category( const item_category *cat ) const
{
    return last_category != cat;
}

int advanced_inventory_pagination::lines_needed( int index )
{
    if( pane.sortby == SORTBY_CATEGORY && new_category( pane.items[index].cat ) ) {
        return 2;
    }
    return 1;
}

bool advanced_inventory_pagination::step( int index )
{
    // item would overflow page
    if( line + lines_needed( index ) > linesPerPage ) {
        page++;
        line = 0;
        // always reprint category header on next page
        last_category = nullptr;
        // print item on next page
        step( index );
        return true;
    }
    // print on this page
    line += lines_needed( index );
    last_category = pane.items[index].cat;
    return false;
}
