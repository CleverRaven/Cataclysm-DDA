#include "gamemode.h"
#include "output.h"

std::string special_game_name(special_game_id id)
{
 switch (id) {
  case SGAME_NULL:
  case NUM_SPECIAL_GAMES:   return "Bug (gamemode.cpp:special_game_name)";
  case SGAME_TUTORIAL:      return _("Tutorial");
  case SGAME_DEFENSE:       return _("Defense");
  default:                  return "Uncoded (gamemode.cpp:special_game_name)";
 }
}

special_game* get_special_game(special_game_id id)
{
 special_game* ret;
 switch (id) {
  case SGAME_NULL:
   ret = new special_game;
   break;
  case SGAME_TUTORIAL:
   ret = new tutorial_game;
   break;
  case SGAME_DEFENSE:
   ret = new defense_game;
   break;
  default:
   debugmsg("Missing something in gamemode.cpp:get_special_game()?");
   ret = new special_game;
   break;
 }

 return ret;
}
