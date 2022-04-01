#include "ui_layout.h"

#include "ui_text_block.h"

void absolute_layout::draw( const catacurses::window &w, const point offset ) const
{
    for( const shared_ptr_fast<ui_element> &element : layout_elements ) {
        element->draw( w, offset + pos );
    }
}

void absolute_layout::add_element( shared_ptr_fast<ui_element> &element )
{
    layout_elements.emplace_back( element );
}

void absolute_layout::remove_element( shared_ptr_fast<ui_element> &element )
{
    layout_elements.erase( std::remove( layout_elements.begin(), layout_elements.end(), element ),
                           layout_elements.end() );
}
