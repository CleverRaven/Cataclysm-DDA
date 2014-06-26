#include "helper.h"
#include "enums.h"

namespace helper
{
    int to_int(std::string str)
    {
        std::istringstream buffer(str);
        int value;
        buffer >> value;
        if(str != to_string_int(value)) //if the converted value is different then return 0 (similar to atoi)
        {
            return 0;
        }
        return value;
    }
    std::string to_string_int(int i)
    {
        std::stringstream sstr;
        sstr << i;
        std::string str1 = sstr.str();
        return str1;
    }
    std::string to_string_long(long i)
    {
        std::stringstream sstr;
        sstr << i;
        std::string str1 = sstr.str();
        return str1;
    }

  Direction movementKeyToDirection(char input) {
    switch (input) {
    case '1':
    case 'b':
    case 'B':
      return Direction::SouthWest;
    case '2':
    case 'j':
    case 'J':
      return Direction::South;
    case '3':
    case 'n':
    case 'N':
      return Direction::SouthEast;
    case '4':
    case 'h':
    case 'H':
      return Direction::West;
    case '5':
    case 'g':
    case 'G':
      return Direction::Center;
    case '6':
    case 'l':
    case 'L':
      return Direction::East;
    case '7':
    case 'y':
    case 'Y':
      return Direction::NorthWest;
    case '8':
    case 'k':
    case 'K':
      return Direction::North;
    case '9':
    case 'u':
    case 'U':
      return Direction::NorthEast;
    case '<':
      return Direction::Up;
    case '>':
      return Direction::Down;
    default:
      return Direction::None;
    }
  }

  tripoint directionToTriPoint(Direction dir) {
    switch (dir) {
    case Direction::SouthWest:
      return tripoint(-1, +1,  0);
    case Direction::South:
      return tripoint( 0, +1 , 0);
    case Direction::SouthEast:
      return tripoint(+1, +1,  0);
    case Direction::West:
      return tripoint(-1,  0,  0);
    case Direction::Center:
    case Direction::None:
      return tripoint( 0,  0,  0);
    case Direction::East:
      return tripoint(+1,  0,  0);
    case Direction::NorthWest:
      return tripoint(-1, -1,  0);
    case Direction::North:
      return tripoint( 0, -1,  0);
    case Direction::NorthEast:
      return tripoint(+1, -1,  0);
    case Direction::Up:
      return tripoint( 0,  0, +1);
    case Direction::Down:
      return tripoint( 0,  0, -1);
    }
  }

  char directionToNumpad(Direction dir) {
    switch (dir) {
    case Direction::SouthWest:
      return '1';
    case Direction::South:
      return '2';
    case Direction::SouthEast:
      return '3';
    case Direction::West:
      return '4';
    case Direction::Center:
      return '5';
    case Direction::East:
      return '6';
    case Direction::NorthWest:
      return '7';
    case Direction::North:
      return '8';
    case Direction::NorthEast:
      return '9';
    case Direction::Up:
      return '<';
    case Direction::Down:
      return '>';
    case Direction::None:
      return '.';
    }
  }

  point directionToPoint (Direction dir) {
    tripoint tmp(directionToTriPoint(dir));

    return tmp.to_point();
  }
}
