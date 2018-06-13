#include "iuse_software_lighting.h"

#include "output.h"
#include "ui.h"
#include "rng.h"
#include "input.h"
#include "catacharset.h"
#include "translations.h"
#include "string_input_popup.h"
#include "lighting.h"
#include "debug.h"

#include <string>
#include <sstream>
#include <vector>
#include <cstdlib>

lighting_game::lighting_game()
{
    iLevelY = 0;
    iLevelX = 0;
}

void lighting_game::new_level( const catacurses::window &w_lighting )
{
    werase( w_lighting );

    mLevel.clear();
    mLevel.resize(iLevelX);
    for (int i = 0; i < iLevelX; i++) {
        mLevel[i].resize(iLevelY);
    }
}

int lighting_game::start_game()
{
    const int iCenterX = ( TERMX > FULL_SCREEN_WIDTH ) ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0;
    const int iCenterY = ( TERMY > FULL_SCREEN_HEIGHT ) ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0;

    catacurses::window w_lighting_border = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                           iCenterY, iCenterX );
    catacurses::window w_lighting = catacurses::newwin( FULL_SCREEN_HEIGHT - 2, FULL_SCREEN_WIDTH - 2,
                                    iCenterY + 1, iCenterX + 1 );

    draw_border( w_lighting_border );

    std::vector<std::string> shortcuts;
    shortcuts.push_back( _( "<n>ew level" ) );
    shortcuts.push_back( _( "<ENTER> change tile" ) );
    shortcuts.push_back( _( "<q>uit" ) );

    int iWidth = 0;
    for( auto &shortcut : shortcuts ) {
        if( iWidth > 0 ) {
            iWidth += 1;
        }
        iWidth += utf8_width( shortcut );
    }

    int iPos = FULL_SCREEN_WIDTH - iWidth - 1;
    for( auto &shortcut : shortcuts ) {
        shortcut_print( w_lighting_border, 0, iPos, c_white, c_light_green, shortcut );
        iPos += utf8_width( shortcut ) + 1;
    }

    mvwputch( w_lighting_border, 0, 2, hilite( c_white ), _( "Experimental Lighting" ) );

    wrefresh( w_lighting_border );

    input_context ctxt( "MINESWEEPER" );
    ctxt.register_cardinal();
    ctxt.register_action( "NEW" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    int iPlayerY = 0;
    int iPlayerX = 0;

    iLevelY = getmaxy( w_lighting );
    iLevelX = getmaxx( w_lighting );

    Lighting lighting(iLevelX, iLevelY);

    int iDirY, iDirX;

    std::string action = "NEW";

    do {
        if( action == "NEW" ) {
            new_level( w_lighting );

            iPlayerX = ( iLevelX / 2 ) + 1;
            iPlayerY = ( iLevelY / 2 ) + 1;
        }

        lighting.recalculateLighting(mLevel, 0.4);

        wclear( w_lighting );

        for (int x = 0; x < iLevelX; x++) {
            for (int y = 0; y < iLevelY; y++) {
                char sGlyph;
                nc_color cColor;

                cColor = c_white;
                if( mLevel[x][y] == Lighting::wall ) {
                    sGlyph = '#';
                    cColor = c_white;
                } else if( mLevel[x][y] == Lighting::lightsource ) {
                    sGlyph = '!';
                    cColor = c_yellow;
                } else {
                    double intensity = lighting.getLight(x, y);
                    sGlyph = intensity > 0 ? intensity : ' ';
                    cColor = c_light_gray;
                }

                mvwputch( w_lighting, y, x, x == iPlayerX && y == iPlayerY ? hilite( cColor ) : cColor, sGlyph );
            }
        }

        wrefresh( w_lighting );

        action = ctxt.handle_input();

        if( ctxt.get_direction( iDirX, iDirY, action ) ) {
            if( iPlayerX + iDirX >= 0 && iPlayerX + iDirX < iLevelX && iPlayerY + iDirY >= 0 &&
                iPlayerY + iDirY < iLevelY ) {
                iPlayerX += iDirX;
                iPlayerY += iDirY;
            }
        } else if( action == "CONFIRM" ) {
            if( mLevel[iPlayerX][iPlayerY] == Lighting::floor ) {
                mLevel[iPlayerX][iPlayerY] = Lighting::wall;

            } else if( mLevel[iPlayerX][iPlayerY] == Lighting::wall ) {
                mLevel[iPlayerX][iPlayerY] = Lighting::lightsource;

            }  else {
                mLevel[iPlayerX][iPlayerY] = Lighting::floor;
            }
        }

    } while( action != "QUIT" );

    return 0;
}
