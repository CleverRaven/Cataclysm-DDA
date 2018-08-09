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
#include <ctime>

lighting_game::lighting_game()
{
    iLevelY = 0;
    iLevelX = 0;
}

void lighting_game::new_level( const catacurses::window &w_lighting )
{
    werase( w_lighting );

    std::for_each(level.begin(), level.end(), [](std::array<int, N_LIGHTING> &in){
        std::fill(in.begin(), in.end(), Lighting::EMPTY);
    });
}

int lighting_game::start_game()
{
    const int iCenterX = ( TERMX > N_LIGHTING + 2 ) ? ( TERMX - N_LIGHTING + 2) / 2 : 0;
    const int iCenterY = ( TERMY > N_LIGHTING + 2) ? ( TERMY - N_LIGHTING + 2) / 2 : 0;

    catacurses::window w_lighting_border = catacurses::newwin( N_LIGHTING+ 2, N_LIGHTING+ 2,
                                           iCenterY, iCenterX );
    catacurses::window w_lighting = catacurses::newwin( N_LIGHTING, N_LIGHTING,
                                    iCenterY + 1, iCenterX + 1 );

    draw_border( w_lighting_border );

    std::vector<std::string> shortcuts;
    shortcuts.push_back( _( "<n>ew level" ) );
    shortcuts.push_back( _( "<F> test" ) );
    shortcuts.push_back( _( "<ENTER> change tile" ) );
    shortcuts.push_back( _( "<q>uit" ) );

    int iWidth = 0;
    for( auto &shortcut : shortcuts ) {
        if( iWidth > 0 ) {
            iWidth += 1;
        }
        iWidth += utf8_width( shortcut );
    }

    int iPos = N_LIGHTING - iWidth - 1;
    for( auto &shortcut : shortcuts ) {
        shortcut_print( w_lighting_border, 0, iPos, c_white, c_light_green, shortcut );
        iPos += utf8_width( shortcut ) + 1;
    }

    mvwputch( w_lighting_border, 0, 2, hilite( c_white ), _( "Experimental Lighting" ) );

    wrefresh( w_lighting_border );

    input_context ctxt( "MINESWEEPER" );
    ctxt.register_cardinal();
    ctxt.register_action( "NEW" );
    ctxt.register_action( "TEST" );
    ctxt.register_action( "EMPTY" );
    ctxt.register_action( "OBSTACLE" );
    ctxt.register_action( "LIGHT" );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_action( "PAGE_UP" );
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

    const float startIntensity = 1;

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

        for (int y = 0; y < iLevelY; y++) {
            for (int x = 0; x < iLevelX; x++) {
                char sGlyph;
                nc_color cColor;

                cColor = c_white;
                if( level[y][x] < Lighting::EMPTY && level[y][x] >= Lighting::OBSTACLE ) {
                    sGlyph = level[y][x] == Lighting::OBSTACLE ? '#' : (level[y][x] * -1) + 48;
                    cColor = c_brown;

                } else if( level[y][x] > Lighting::EMPTY && level[y][x] <= Lighting::LIGHT_SOURCE ) {
                    sGlyph = level[y][x] == Lighting::LIGHT_SOURCE ? '!' : level[y][x] + 48;
                    cColor = c_yellow_red;

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
        } else if( action == "TEST" ) {
            std::for_each(level.begin(), level.end(), [](std::array<int, N_LIGHTING> &in){
                std::fill(in.begin(), in.end(), Lighting::LIGHT_SOURCE);
            });

            std::clock_t start;
            double duration;

            start = std::clock();

            for (int i=0; i < 10; i++) {
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
            }

            duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;

            debugmsg("Total 10 tests: %1.10f - Average: %1.10f", duration, duration / 10);

        } else if( action == "EMTPY" ) {
            level[iPlayerY][iPlayerX] = Lighting::EMPTY;

        } else if( action == "OBSTACLE" ) {
            level[iPlayerY][iPlayerX] = Lighting::OBSTACLE;

        } else if( action == "LIGHT" ) {
            level[iPlayerY][iPlayerX] = Lighting::LIGHT_SOURCE;

        } else if( action == "PAGE_DOWN" ) {
            level[iPlayerY][iPlayerX] -= 1;

            if ( level[iPlayerY][iPlayerX] < Lighting::OBSTACLE ) {
                level[iPlayerY][iPlayerX] = Lighting::EMPTY;
            }

        } else if( action == "PAGE_UP" ) {
            level[iPlayerY][iPlayerX] += 1;

            if ( level[iPlayerY][iPlayerX] > Lighting::LIGHT_SOURCE ) {
                level[iPlayerY][iPlayerX] = Lighting::EMPTY;
            }
        }

    } while( action != "QUIT" );

    return 0;
}
