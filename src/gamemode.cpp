#include "gamemode.h"

#include "debug.h"
#include "output.h"
#include "translations.h"

std::string special_game_name( special_game_id id )
{
    switch( id ) {
        case SGAME_NULL:
        case NUM_SPECIAL_GAMES:
            return "Bug (gamemode.cpp:special_game_name)";
        case SGAME_TUTORIAL:
            return _( "Tutorial" );
        case SGAME_DEFENSE:
            return _( "Defense" );
        default:
            return "Uncoded (gamemode.cpp:special_game_name)";
    }
}

std::unique_ptr<special_game> get_special_game( special_game_id id )
{
    std::unique_ptr<special_game> ret;
    switch( id ) {
        case SGAME_NULL:
            ret.reset( new special_game );
            break;
        case SGAME_TUTORIAL:
            ret.reset( new tutorial_game );
            break;
        case SGAME_DEFENSE:
            ret.reset( new defense_game );
            break;
        default:
            debugmsg( "Missing something in gamemode.cpp:get_special_game()?" );
            ret.reset( new special_game );
            break;
    }

    return ret;
}
