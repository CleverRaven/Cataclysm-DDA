#ifndef _MORALE_H_
#define _MORALE_H_

#include <string>
#include "moraledata.h"

#define MIN_MORALE_READ		(-20)
#define MIN_MORALE_CRAFT	(-15)

enum morale_type {
 MOR_NULL,
 MOR_MOODSWING_BAD,
 MOR_MOODSWING_GOOD,
 MOR_FEEL_BAD,
 MOR_FEEL_GOOD,
 MOR_FOOD_BAD,
 MOR_FOOD_GOOD,
 MOR_FOOD_VEGETARIAN,
 MOR_BOOK_BAD,
 MOR_BOOK_GOOD,
 MOR_MUSIC,
 MOR_KILLED_NEUTRAL,
 MOR_KILLED_FRIEND,
 MOR_MONSTER_GUILT,

 MOR_CRAVING_CAFFEINE,
 MOR_CRAVING_NICOTINE,
 MOR_CRAVING_ALCOHOL,
 MOR_CRAVING_COCAINE,
 MOR_CRAVING_SPEED,
 MOR_CRAVING_OPIATE,
 MOR_MAX
};


struct morale_point
{
 morale_type type;
 int bonus;
 int duration;

 morale_point(morale_type T = MOR_NULL, int B = 0, int D = 0) :
              type (T), bonus (B), duration (D) {};

 std::string name() { return morale_data[type].name; };

 void up_bonus(int b) {
  int max = morale_data[type].top_bonus;
  if (max < 0 && bonus > 0)
   bonus -= b;
  else
   bonus += b;
  if ((max < 0 && bonus < max) || (max > 0 && bonus > max))
   bonus = max;
 }

 void up_duration(int d) {
  int max = morale_data[type].top_duration;
  if (max < 0 && duration > 0)
   duration -= d;
  else
   duration += d;
  if ((max < 0 && duration < max) || (max > 0 && duration > max))
   duration = max;
 }
};
 
#endif
