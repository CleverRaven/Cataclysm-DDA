#pragma once
#ifndef ENUMS_H
#define ENUMS_H

#include <utility>
#include <climits>
#include <ostream>

class JsonOut;
class JsonIn;

template<typename T>
constexpr inline int sgn( const T x )
{
    return x < 0 ? -1 : ( x > 0 ? 1 : 0 );
}

// By default unordered_map doesn't have a hash for tuple or pairs, so we need to include some.
// This is taken almost directly from the boost library code.
// Function has to live in the std namespace
// so that it is picked up by argument-dependent name lookup (ADL).
namespace std{
    namespace
    {

        // Code from boost
        // Reciprocal of the golden ratio helps spread entropy
        //     and handles duplicates.
        // See Mike Seymour in magic-numbers-in-boosthash-combine:
        //     http://stackoverflow.com/questions/4948780

        template <class T>
        inline void hash_combine(std::size_t& seed, T const& v)
        {
            seed ^= hash<T>()(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        }

        // Recursive template code derived from Matthieu M.
        template <class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
        struct HashValueImpl
        {
            static void apply(size_t& seed, Tuple const& tuple)
            {
                HashValueImpl<Tuple, Index-1>::apply(seed, tuple);
                hash_combine(seed, get<Index>(tuple));
            }
        };

        template <class Tuple>
        struct HashValueImpl<Tuple,0>
        {
            static void apply(size_t& seed, Tuple const& tuple)
            {
                hash_combine(seed, get<0>(tuple));
            }
        };
    }

    template <typename ... TT>
    struct hash<std::tuple<TT...>>
    {
        size_t
        operator()(std::tuple<TT...> const& tt) const
        {
            size_t seed = 0;
            HashValueImpl<std::tuple<TT...> >::apply(seed, tt);
            return seed;
        }

    };

    template <class A, class B>
    struct hash<std::pair<A, B>>
    {
        std::size_t operator() (const std::pair<A, B>& v) const {
            std::size_t seed = 0;
            hash_combine(seed, v.first);
            hash_combine(seed, v.second);
            return seed;
        }
    };
}

//Used for autopickup and safemode rules
enum rule_state : int {
    RULE_NONE,
    RULE_WHITELISTED,
    RULE_BLACKLISTED
};

enum visibility_type {
  VIS_HIDDEN,
  VIS_CLEAR,
  VIS_LIT,
  VIS_BOOMER,
  VIS_DARK,
  VIS_BOOMER_DARK
};

enum special_game_id : int {
    SGAME_NULL = 0,
    SGAME_TUTORIAL,
    SGAME_DEFENSE,
    NUM_SPECIAL_GAMES
};

enum art_effect_passive : int {
    AEP_NULL = 0,
    // Good
    AEP_STR_UP, // Strength + 4
    AEP_DEX_UP, // Dexterity + 4
    AEP_PER_UP, // Perception + 4
    AEP_INT_UP, // Intelligence + 4
    AEP_ALL_UP, // All stats + 2
    AEP_SPEED_UP, // +20 speed
    AEP_PBLUE, // Reduces radiation
    AEP_SNAKES, // Summons friendly snakes when you're hit
    AEP_INVISIBLE, // Makes you invisible
    AEP_CLAIRVOYANCE, // See through walls
    AEP_SUPER_CLAIRVOYANCE, // See through walls to a great distance
    AEP_STEALTH, // Your steps are quieted
    AEP_EXTINGUISH, // May extinguish nearby flames
    AEP_GLOW, // Four-tile light source
    AEP_PSYSHIELD, // Protection from stare attacks
    AEP_RESIST_ELECTRICITY, // Protection from electricity
    AEP_CARRY_MORE, // Increases carrying capacity by 200
    AEP_SAP_LIFE, // Killing non-zombie monsters may heal you
    AEP_FUN, // Slight passive morale
    // Splits good from bad
    AEP_SPLIT,
    // Bad
    AEP_HUNGER, // Increases hunger
    AEP_THIRST, // Increases thirst
    AEP_SMOKE, // Emits smoke occasionally
    AEP_EVIL, // Addiction to the power
    AEP_SCHIZO, // Mimicks schizophrenia
    AEP_RADIOACTIVE, // Increases your radiation
    AEP_MUTAGENIC, // Mutates you slowly
    AEP_ATTENTION, // Draws netherworld attention slowly
    AEP_STR_DOWN, // Strength - 3
    AEP_DEX_DOWN, // Dex - 3
    AEP_PER_DOWN, // Per - 3
    AEP_INT_DOWN, // Int - 3
    AEP_ALL_DOWN, // All stats - 2
    AEP_SPEED_DOWN, // -20 speed
    AEP_FORCE_TELEPORT, // Occasionally force a teleport
    AEP_MOVEMENT_NOISE, // Makes noise when you move
    AEP_BAD_WEATHER, // More likely to experience bad weather
    AEP_SICK, // Decreases health over time
    AEP_CLAIRVOYANCE_PLUS, // See through walls to a larger distance; not bad effect, placement to preserve old saves.

    NUM_AEPS
};

enum artifact_natural_property {
    ARTPROP_NULL,
    ARTPROP_WRIGGLING, //
    ARTPROP_GLOWING, //
    ARTPROP_HUMMING, //
    ARTPROP_MOVING, //
    ARTPROP_WHISPERING, //
    ARTPROP_BREATHING, //
    ARTPROP_DEAD, //
    ARTPROP_ITCHY, //
    ARTPROP_GLITTERING, //
    ARTPROP_ELECTRIC, //
    ARTPROP_SLIMY, //
    ARTPROP_ENGRAVED, //
    ARTPROP_CRACKLING, //
    ARTPROP_WARM, //
    ARTPROP_RATTLING, //
    ARTPROP_SCALED,
    ARTPROP_FRACTAL,
    ARTPROP_MAX
};

enum phase_id : int {
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

struct point {
    int x;
    int y;
    point() : x(0), y(0) {}
    point(int X, int Y) : x (X), y (Y) {}

    point operator+(const point &rhs) const
    {
        return point( x + rhs.x, y + rhs.y );
    }
    point &operator+=(const point &rhs)
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }
    point operator-(const point &rhs) const
    {
        return point( x - rhs.x, y - rhs.y );
    }
    point &operator-=(const point &rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }
};

void serialize( const point &p, JsonOut &jsout );
void deserialize( point &p, JsonIn &jsin );

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
    tripoint() : x(0), y(0), z(0) {}
    tripoint(int X, int Y, int Z) : x (X), y (Y), z (Z) {}
    explicit tripoint(const point &p, int Z) : x (p.x), y (p.y), z (Z) {}

    tripoint operator+(const tripoint &rhs) const
    {
        return tripoint( x + rhs.x, y + rhs.y, z + rhs.z );
    }
    tripoint operator-(const tripoint &rhs) const
    {
        return tripoint( x - rhs.x, y - rhs.y, z - rhs.z );
    }
    tripoint &operator+=(const tripoint &rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }
    tripoint operator-() const
    {
        return tripoint( -x, -y, -z );
    }
    /*** some point operators and functions ***/
    tripoint operator+(const point &rhs) const
    {
        return tripoint(x + rhs.x, y + rhs.y, z);
    }
    tripoint operator-(const point &rhs) const
    {
        return tripoint(x - rhs.x, y - rhs.y, z);
    }
    tripoint &operator+=(const point &rhs)
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }
    tripoint &operator-=(const point &rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }
    tripoint &operator-=( const tripoint &rhs )
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );
};

inline std::ostream &operator<<( std::ostream &os, const tripoint &pos )
{
    return os << pos.x << "," << pos.y << "," << pos.z;
}

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

static const tripoint tripoint_min { INT_MIN, INT_MIN, INT_MIN };
static const tripoint tripoint_zero { 0, 0, 0 };

struct sphere
{
    int radius;
    tripoint center;

    sphere() : radius( 0 ), center() {}
    explicit sphere( const tripoint &center ) : radius( 1 ), center( center ) {}
    explicit sphere( const tripoint &center, int radius ) : radius( radius ), center( center ) {}
};

#endif
