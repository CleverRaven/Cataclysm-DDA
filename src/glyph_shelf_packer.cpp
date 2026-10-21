#include "glyph_shelf_packer.h"

int next_power_of_two( const int value )
{
    int p = 1;
    while( p < value ) {
        p *= 2;
    }
    return p;
}

glyph_shelf_packer::glyph_shelf_packer( const int page_size, const int cell_height,
                                        const int gap )
    : page_size_( page_size ), cell_height_( cell_height ), gap_( gap )
{
}

std::optional<glyph_shelf_packer::placement> glyph_shelf_packer::plan( const int width ) const
{
    if( width <= 0 || cell_height_ > page_size_ ) {
        return std::nullopt;
    }
    placement p;
    p.next_current = current_;
    p.next = cursor_;
    const int new_page = static_cast<int>( pages_.size() );
    if( width > page_size_ ) {
        // own page; the regular shelf continues where it was
        p.at = slot{ new_page, point::zero };
        p.page_w = next_power_of_two( width );
        p.page_h = next_power_of_two( cell_height_ );
        p.opens_page = true;
        return p;
    }
    point at = cursor_;
    if( current_ >= 0 && at.x + width > page_size_ ) {
        at = point( 0, at.y + cell_height_ + gap_ );
    }
    int page = current_;
    if( current_ < 0 || at.y + cell_height_ > page_size_ ) {
        page = new_page;
        at = point::zero;
        p.opens_page = true;
    }
    p.at = slot{ page, at };
    p.page_w = p.opens_page ? page_size_ : pages_[page].w;
    p.page_h = p.opens_page ? page_size_ : pages_[page].h;
    p.next_current = page;
    p.next = at + point( width + gap_, 0 );
    return p;
}

void glyph_shelf_packer::commit( const placement &p )
{
    if( p.opens_page ) {
        pages_.push_back( page_dims{ p.page_w, p.page_h } );
    }
    current_ = p.next_current;
    cursor_ = p.next;
}

std::optional<glyph_shelf_packer::slot> glyph_shelf_packer::place( const int width )
{
    const std::optional<placement> p = plan( width );
    if( !p ) {
        return std::nullopt;
    }
    commit( *p );
    return p->at;
}

int glyph_shelf_packer::page_count() const
{
    return static_cast<int>( pages_.size() );
}

int glyph_shelf_packer::page_width( const int page ) const
{
    return pages_.at( page ).w;
}

int glyph_shelf_packer::page_height( const int page ) const
{
    return pages_.at( page ).h;
}

void glyph_shelf_packer::clear()
{
    pages_.clear();
    current_ = -1;
    cursor_ = point::zero;
}
