#ifndef _HELPER_H_
#define _HELPER_H_
#include <string>
#include <sstream>

#include <vector>

struct tripoint;
struct point;

namespace helper
{
    int to_int(std::string str);
    std::string to_string_int(int i);
    std::string to_string_long(long i);

    enum class Direction {
      None = 0,
      Center, East, West, North, South,
      NorthEast, NorthWest, SouthEast, SouthWest,
      Up, Down
    };

    const std::vector<Direction> planarDirections = {
      Direction::SouthWest,
      Direction::South,
      Direction::SouthEast,
      Direction::West,
      Direction::Center,
      Direction::East,
      Direction::NorthWest,
      Direction::North,
      Direction::NorthEast,
    };

    Direction movementKeyToDirection (char);
    point directionToPoint (Direction);
    tripoint directionToTriPoint (Direction);
    char directionToNumpad (Direction);

    Direction pointToDirection (const point &);

    double convertWeight(int);
}
#endif
