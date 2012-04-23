#include "gamemode.h"
#include "output.h"

std::string special_game_name(special_game_id id)
{
 switch (id) {
  case SGAME_NULL:
  case NUM_SPECIAL_GAMES:	return "nethack (this is a bug)";
  case SGAME_TUTORIAL:		return "Tutorial";
  case SGAME_DEFENSE:		return "Defense";
  default:			return "Uncoded (this is a bug)";
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
   debugmsg("Missing something in get_special_game()?");
   ret = new special_game;
   break;
 }

 return ret;
}
