#ifndef _ENUMS_H_
#define _ENUMS_H_

#include "json.h" // (de)serialization for points

#ifndef sgn
#define sgn(x) (((x) < 0) ? -1 : 1)
#endif

enum phase_id {
PNULL, SOLID, LIQUID, GAS, PLASMA
};

// Return the class an in-world object uses to interact with the world.
//   ex; if ( player.grab_type == OBJECT_VEHICLE ) { ...
//   or; if ( baseactor_just_shot_at.object_type() == OBJECT_NPC ) { ...
enum object_type {
  OBJECT_NONE,      // Nothing, invalid.
  OBJECT_ITEM,      // item.h
  OBJECT_ACTOR,     // potential virtual base class, get_object_type() would return one of the types below
    OBJECT_PLAYER,  // player.h, npc.h
      OBJECT_NPC,   // nph.h
    OBJECT_MONSTER, // monster.h
  OBJECT_VEHICLE,   // vehicle.h
  OBJECT_TRAP,      // trap.h
  OBJECT_FIELD,     // field.h; field_entry
  OBJECT_TERRAIN,   // Not a real object
  OBJECT_FURNITURE, // Not a real object
  NUM_OBJECTS,
};

enum damage_type
{
    DT_NULL = 0, // null damage, doesn't exist
    DT_NONE, // typeless damage, should always go through
    DT_INTERNAL, // internal damage, like from smoke or poison
    DT_BASH, // bash damage
    DT_CUT, // cut damage
    DT_ACID, // corrosive damage, e.g. acid
    DT_STAB, // stabbing/piercing damage
    DT_HEAT, // e.g. fire, plasma
    DT_ELECTRIC, // e.g. electrical discharge
    NUM_DT
};

struct point : public JsonSerializer, public JsonDeserializer
{
    int x;
    int y;
    point(int X = 0, int Y = 0) : x (X), y (Y) {}
    point(const point &p) : JsonSerializer(), JsonDeserializer(), x (p.x), y (p.y) {}
    ~point(){}
    using JsonSerializer::serialize;
    void serialize(JsonOut &jsout) const {
        jsout.start_array();
        jsout.write(x);
        jsout.write(y);
        jsout.end_array();
    }
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin) {
        JsonArray ja = jsin.get_array();
        x = ja.get_int(0);
        y = ja.get_int(1);
    }
};

inline bool operator<(const point& a, const point& b)
{
  return a.x < b.x || (a.x == b.x && a.y < b.y);
}

struct tripoint {
 int x;
 int y;
 int z;
 tripoint(int X = 0, int Y = 0, int Z = 0) : x (X), y (Y), z (Z) {}
 tripoint(const tripoint &p) : x (p.x), y (p.y), z (p.z) {}
 ~tripoint(){}
};

#endif
