#include "game.h"
#include "player.h"
#include "monster.h"
#include "map.h"
#include "mapdata.h"
#include "input.h"
#include "ui.h"

enum class description_target : int {
    creature,
    furniture,
    terrain
};

void game::extended_description( const tripoint &p )
{
    const int left = 0;
    const int right = TERMX;
    const int top = 3;
    const int bottom = TERMY;
    const int width = right - left;
    const int height = bottom - top;
    WINDOW *w_head = newwin( top, TERMX, 0, 0 );
    WINDOW *w_main = newwin( height, width, top, left );
    // @todo De-hardcode
    std::string header_message = _( "\
c to describe creatures, f to describe furniture, t to describe terrain, esc/enter to close." );
    description_target cur_target = description_target::creature;
    mvwprintz( w_head, 0, 0, c_white, header_message.c_str() );

    // Set up line drawings
    for( int i = 0; i < TERMX; i++ ) {
        mvwputch( w_head, top - 1, i, c_white, LINE_OXOX );
    }

    wrefresh( w_head );
    int ch = 'c';
    do {
        std::string desc;
        // Allow looking at invisible tiles - player may want to examine hallucinations etc.
        switch( cur_target ) {
            case description_target::creature:
            {
                const Creature *critter = critter_at( p );
                if( critter != nullptr && u.sees( *critter ) ) {
                    desc = critter->extended_description();
                } else {
                    desc = _( "You do not see any creature here." );
                }
            }
                break;
            case description_target::furniture:
                if( !u.sees( p ) || !m.has_furn( p ) ) {
                    desc = _( "You do not see any furniture here." );
                } else {
                    const furn_id fid = m.furn( p );
                    desc = fid.obj().description();
                }
                break;
            case description_target::terrain:
                if( !u.sees( p ) ) {
                    desc = _( "You can't see the terrain here." );
                } else {
                    const ter_id tid = m.ter( p );
                    desc = tid.obj().description();
                }
                break;
        }

        werase( w_main );
        fold_and_print_from( w_main, 0, 0, width, 0, c_ltgray, desc );
        wrefresh( w_main );
        // TODO: use input context
        ch = inp_mngr.get_input_event().get_first_input();
        switch( ch ) {
            case 'c':
                cur_target = description_target::creature;
                break;
            case 'f':
                cur_target = description_target::furniture;
                break;
            case 't':
                cur_target = description_target::terrain;
                break;
        }

    } while( ch != KEY_ESCAPE && ch != '\n' );

    werase( w_head );
    werase( w_main );
    wrefresh( w_head );
    wrefresh( w_main );
    delwin( w_head );
    delwin( w_main );
}