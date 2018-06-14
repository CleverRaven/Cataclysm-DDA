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
#include <algorithm>

lighting_game::lighting_game()
{
    iLevelY = 0;
    iLevelX = 0;
}

void lighting_game::new_level( const catacurses::window &w_lighting )
{
    werase( w_lighting );

    std::for_each(level.begin(), level.end(), [](std::array<int, N_LIGHTING> &in){
        std::fill(in.begin(), in.end(), 0);
    });
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

    iLevelY = N_LIGHTING;
    iLevelX = N_LIGHTING;

    Lighting lighting;
    Lighting lighting_ccw;
    Lighting lighting_cw;
    Lighting lighting_pi;

    int iDirY, iDirX;

    std::string action = "NEW";

    const float startIntensity = 2.0;

    do {
        if( action == "NEW" ) {
            new_level( w_lighting );

            iPlayerX = ( iLevelX / 2 ) + 1;
            iPlayerY = ( iLevelY / 2 ) + 1;
        }

        lighting.setInputRotated(level, Lighting::ROT_NO);
        lighting.recalculateLighting(startIntensity);

        lighting_ccw.setInputRotated(level, Lighting::ROT_CCW);
        lighting_ccw.recalculateLighting(startIntensity);
        lighting_ccw.accumulateLightRotated(lighting, Lighting::ROT_CW);

        lighting_cw.setInputRotated(level, Lighting::ROT_CW);
        lighting_cw.recalculateLighting(startIntensity);
        lighting_cw.accumulateLightRotated(lighting, Lighting::ROT_CCW);

        lighting_pi.setInputRotated(level, Lighting::ROT_PI);
        lighting_pi.recalculateLighting(startIntensity);
        lighting_pi.accumulateLightRotated(lighting, Lighting::ROT_PI);

        wclear( w_lighting );

        //std::stringstream ss;

        for (int y = 0; y < N_LIGHTING; y++) {
            for (int x = 0; x < N_LIGHTING; x++) {
                char sGlyph;
                nc_color cColor;

                cColor = c_white;
                if( level[y][x] == Lighting::OBSTACLE ) {
                    sGlyph = '#';
                    cColor = c_white;
                } else if( level[y][x] == Lighting::LIGHT_SOURCE ) {
                    sGlyph = '!';
                    cColor = c_yellow;
                } else {
                    int intensity = lighting.getLight(y, x) + 32;

                    //ss << intensity << " ";

                    sGlyph = intensity > 0 ? intensity : ' ';
                    cColor = c_light_gray;
                }

                mvwputch( w_lighting, y, x, x == iPlayerX && y == iPlayerY ? hilite( cColor ) : cColor, sGlyph );
            }
        }

        //debugmsg("%s", ss.str());

        wrefresh( w_lighting );

        action = ctxt.handle_input();

        if( ctxt.get_direction( iDirX, iDirY, action ) ) {
            if( iPlayerX + iDirX >= 0 && iPlayerX + iDirX < iLevelX && iPlayerY + iDirY >= 0 &&
                iPlayerY + iDirY < iLevelY ) {
                iPlayerX += iDirX;
                iPlayerY += iDirY;
            }
        } else if( action == "CONFIRM" ) {
            if( level[iPlayerY][iPlayerX] == Lighting::EMPTY ) {
                level[iPlayerY][iPlayerX] = Lighting::OBSTACLE;

            } else if( level[iPlayerY][iPlayerX] == Lighting::OBSTACLE ) {
                level[iPlayerY][iPlayerX] = Lighting::LIGHT_SOURCE;

            }  else {
                level[iPlayerY][iPlayerX] = Lighting::EMPTY;
            }
        }

    } while( action != "QUIT" );

    return 0;
}
