#include <vector>
#include <string>
#include "crafting.h"

struct construct;

struct construction_stage
{
 ter_id terrain;
 int time; // In minutes, i.e. 10 turns
 std::vector<itype_id> tools[3];
 std::vector<component> components[3];

 construction_stage(ter_id Terrain, int Time) :
    terrain (Terrain), time (Time) { };
};

struct constructable
{
 int id;
 std::string name; // Name as displayed
 int difficulty; // Carpentry skill level required
 std::vector<construction_stage> stages;
 bool (construct::*able)  (game *, point);

 constructable(int Id, std::string Name, int Diff,
               bool (construct::*Able) (game *, point)) :
  id (Id), name (Name), difficulty (Diff), able (Able) {};
};

struct construct // Construction functions.
{
 bool able_always(game *, point) { return true;  }
 bool able_never (game *, point) { return false; }
 bool able_empty (game *, point); // Able if tile is empty
 bool able_wall  (game *, point); // Able if tile is wall
 bool able_between_walls(game *, point); // Flood-fill contained by walls
};
