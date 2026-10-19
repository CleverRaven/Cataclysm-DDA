#pragma once
#ifndef CATA_SRC_GLYPH_SHELF_PACKER_H
#define CATA_SRC_GLYPH_SHELF_PACKER_H

#include <optional>
#include <vector>

#include "point.h"

// Smallest power of two not below `value` (value >= 1)
int next_power_of_two( int value );

// Place cells of one fixed height into square pages row by row
class glyph_shelf_packer
{
    public:
        struct slot {
            int page = 0;
            point pos;
        };
        // where cell would go and packer state after taking it
        struct placement {
            slot at;
            // dimensions of the target page, known before the page exists
            int page_w = 0;
            int page_h = 0;
            bool opens_page = false;
            int next_current = -1;
            point next;
        };
        // page_size is a regular page's side, a power of two; gap is the empty
        // texels kept right of and below each cell
        glyph_shelf_packer( int page_size, int cell_height, int gap );
        // slot a cell of `width` would take, without taking it. a cell wider
        // than a page gets its own page; nullopt for width <= 0
        std::optional<placement> plan( int width ) const;
        // take the slot returned by plan(); no other call may come in between
        void commit( const placement &p );
        // plan() and commit() in one step.
        std::optional<slot> place( int width );
        int page_count() const;
        int page_width( int page ) const;
        int page_height( int page ) const;
        void clear();

    private:
        struct page_dims {
            int w;
            int h;
        };
        int page_size_;
        int cell_height_;
        int gap_;
        std::vector<page_dims> pages_;
        // regular page being filled, or -1 before the first
        int current_ = -1;
        // where the next cell goes on the current page
        point cursor_;
};

#endif // CATA_SRC_GLYPH_SHELF_PACKER_H
