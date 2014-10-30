#ifndef ENUMS_H
#define ENUMS_H

#include "json.h" // (de)serialization for points

#ifndef sgn
#define sgn(x) (((x) < 0) ? -1 : 1)
#endif

enum special_game_id {
    SGAME_NULL = 0,
    SGAME_TUTORIAL,
    SGAME_DEFENSE,
    NUM_SPECIAL_GAMES
};

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

struct point : public JsonSerializer, public JsonDeserializer {
    int x;
    int y;
    point(int X = 0, int Y = 0) : x (X), y (Y) {}
    point(point &&) = default;
    point(const point &) = default;
    point &operator=(point &&) = default;
    point &operator=(const point &) = default;
    ~point() {}
    using JsonSerializer::serialize;
    void serialize(JsonOut &jsout) const
    {
        jsout.start_array();
        jsout.write(x);
        jsout.write(y);
        jsout.end_array();
    }
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin)
    {
        JsonArray ja = jsin.get_array();
        x = ja.get_int(0);
        y = ja.get_int(1);
    }
};

// Make point hashable so it can be used as an unordered_set or unordered_map key,
// or a component of one.
namespace std {
  template <>
  struct hash<point> {
      std::size_t operator()(const point& k) const {
          // Circular shift y by half its width so hash(5,6) != hash(6,5).
          return std::hash<int>()(k.x) ^ std::hash<int>()( (k.y << 16) | (k.y >> 16) );
      }
  };
}

inline bool operator<(const point &a, const point &b)
{
    return a.x < b.x || (a.x == b.x && a.y < b.y);
}
inline bool operator==(const point &a, const point &b)
{
    return a.x == b.x && a.y == b.y;
}
inline bool operator!=(const point &a, const point &b)
{
    return !(a == b);
}

struct tripoint {
    int x;
    int y;
    int z;
    tripoint(int X = 0, int Y = 0, int Z = 0) : x (X), y (Y), z (Z) {}
};

// Make tripoint hashable so it can be used as an unordered_set or unordered_map key,
// or a component of one.
namespace std {
  template <>
  struct hash<tripoint> {
      std::size_t operator()(const tripoint& k) const {
          // Circular shift y and z so hash(5,6,7) != hash(7,6,5).
          return std::hash<int>()(k.x) ^
              std::hash<int>()( (k.y << 10) | (k.y >> 10) ) ^
              std::hash<int>()( (k.z << 20) | (k.z >> 20) );
      }
  };
}

inline bool operator==(const tripoint &a, const tripoint &b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
inline bool operator!=(const tripoint &a, const tripoint &b)
{
    return !(a == b);
}
inline bool operator<(const tripoint &a, const tripoint &b)
{
    if (a.x != b.x) {
        return a.x < b.x;
    }
    if (a.y != b.y) {
        return a.y < b.y;
    }
    if (a.z != b.z) {
        return a.z < b.z;
    }
    return false;
}

#endif
