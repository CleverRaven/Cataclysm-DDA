#include "ui_text_block.h"

#include "output.h"

void text_block::draw( const catacurses::window &w, const point offset ) const
{
    if( !multiline ) {
        trim_and_print( w, offset + pos, width, color, text );
    } else {
        // todo: scrolling and/or some indicator text is cut off
        // or possibly dynamic height (set height to lines.size())
        std::vector<std::string> lines = foldstring( text, width );
        int max_lines = std::min( static_cast<int>( lines.size() ), height );
        for( int i = 0; i < max_lines; i++ ) {
            nc_color dummy;
            print_colored_text( w, offset + pos + point( 0, i ), dummy, color, text );
        }
    }
}
