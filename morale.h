#ifndef _MORALE_H_
#define _MORALE_H_

#include <string>

#define MIN_MORALE_READ		(-40)
#define MIN_MORALE_CRAFT	(-50)

enum morale_type
{
 MORALE_NULL = 0,
 MORALE_FOOD_GOOD,
 MORALE_MUSIC,
 MORALE_MARLOSS,
 MORALE_FEELING_GOOD,

 MORALE_CRAVING_NICOTINE,
 MORALE_CRAVING_CAFFEINE,
 MORALE_CRAVING_ALCOHOL,
 MORALE_CRAVING_OPIATE,
 MORALE_CRAVING_SPEED,
 MORALE_CRAVING_COCAINE,

 MORALE_FOOD_BAD,
 MORALE_VEGETARIAN,
 MORALE_WET,
 MORALE_FEELING_BAD,
 MORALE_KILLED_INNOCENT,
 MORALE_KILLED_FRIEND,
 MORALE_KILLED_MONSTER,

 MORALE_MOODSWING,
 MORALE_BOOK,

 MORALE_SCREAM,

 NUM_MORALE_TYPES
};

struct morale_point
{
 morale_type type;
 itype* item_type;
 int bonus;

 morale_point(morale_type T = MORALE_NULL, itype* I = NULL, int B = 0) :
              type (T), item_type (I), bonus (B) {};

 std::string name(std::string morale_data[])
 {
  std::string ret = morale_data[type];
  std::string item_name = "";
  if (item_type != NULL)
   item_name = item_type->name;
  size_t it = ret.find("%i");
  while (it != std::string::npos) {
   ret.replace(it, 2, item_name);
   it = ret.find("%i");
  }
  return ret;
 }
};
 
#endif
