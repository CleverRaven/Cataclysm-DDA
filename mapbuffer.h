#include "map.h"
#include "line.h"
#include <map>
#include <list>

class game;

struct pointcomp
{
 bool operator() (const tripoint &lhs, const tripoint &rhs) const
 {
  if (lhs.x < rhs.x) return true;
  if (lhs.x > rhs.x) return false;
  if (lhs.y < rhs.y) return true;
  if (lhs.y > rhs.y) return false;
  if (lhs.z < rhs.z) return true;
  if (lhs.z > rhs.z) return false;
  return false;
 };
};

class mapbuffer
{
 public:
  mapbuffer(game *g = NULL);
  ~mapbuffer();

  void set_game(game *g);

  void load();
  void save();

  bool add_submap(int x, int y, int z, submap *sm);
  submap* lookup_submap(int x, int y, int z);

  int size();

 private:
  std::map<tripoint, submap*, pointcomp> submaps;
  std::list<submap*> submap_list;
  game *master_game;
};
  
extern mapbuffer MAPBUFFER;
