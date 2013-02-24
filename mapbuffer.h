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
  mapbuffer();
  ~mapbuffer();

  void set_game(game *g);

  //help save_if_dirty() know to save as many times as it's supposed to.
  void set_dirty();
  void make_volatile();

  void load();
  void save();
  void save_if_dirty();
  void reset();

  bool add_submap(int x, int y, int z, submap *sm);
  submap* lookup_submap(int x, int y, int z);

  int size();

 private:
  std::map<tripoint, submap*, pointcomp> submaps;
  std::list<submap*> submap_list;
  game *master_game;
  bool dirty;
};

extern mapbuffer MAPBUFFER;
