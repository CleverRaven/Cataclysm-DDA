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
 void (construct::*done)  (game *, point);

 constructable(int Id, std::string Name, int Diff,
               bool (construct::*Able) (game *, point),
               void (construct::*Done) (game *, point)) :
  id (Id), name (Name), difficulty (Diff), able (Able), done (Done) {};
};

struct construct // Construction functions.
{
// Bools - able to build at the given point?
 bool able_always(game *, point) { return true;  }
 bool able_never (game *, point) { return false; }

 bool able_empty (game *, point); // Able if tile is empty

 bool able_window(game *, point); // Any window tile
 bool able_window_pane(game *, point); // Only intact windows
 bool able_broken_window(game *, point); // Able if tile is broken window

 bool able_door(game *, point); // Any door tile
 bool able_door_broken(game *, point); // Broken door

 bool able_wall  (game *, point); // Able if tile is wall
 bool able_wall_wood(game *g, point); // Only player-built walls

 bool able_between_walls(game *, point); // Flood-fill contained by walls

 bool able_dig(game *, point); // Able if diggable terrain
 bool able_pit(game *, point); // Able only on pits

 bool able_tree(game *, point); // Able on trees
 bool able_log(game *, point); // Able on logs

// Does anything special happen when we're finished?
 void done_nothing(game *, point) { }
 void done_window_pane(game *, point);
 void done_vehicle(game *, point);
 void done_tree(game *, point);
 void done_log(game *, point);

};
