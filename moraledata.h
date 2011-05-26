#ifndef _MORALEDATA_H_
#define _MORALEDATA_H_

#include <string>

struct morale_datum
{
 std::string name;
 int top_bonus;
 int top_duration;
};

#ifndef _MORALE_DATA_
#define _MORALE_DATA_
const morale_datum morale_data[] = {
{"ERROR - Null",	   0,	     0},
{"Bad Mood Swing",	-100,	  1200},
{"Good Mood Swing",	 100,	  1200},
{"Bad Feeling",		 -20,	   600},
{"Good Feeling",	  20,	   600},
{"Disgusting Food",	-100,	   900},
{"Delicious Food",	  80,	   900},
{"Ate Meat",		 -50,	  1200},
{"Dull Reading",	 -15,	   600},
{"Fun Reading",		  20,	   900},
{"Listened to Music",	  50,	   450},
{"Killed Innocent",	 -80,	 14400},	// One day
{"Killed Friend",	-600,	100800},	// One week!
{"Killed Mom",		 -10,	   900},
{"Religious Experience", 300,    28800},

{"Caffeine Craving",	 -10,	  1800},
{"Nicotine Craving",	 -20,	  2200},
{"Alcohol Craving",	 -80,	  3600},
{"Cocaine Craving",	 -36,	  2600},
{"Amphetamine Craving",	 -50,	  3000},
{"Opiate Craving",	-200,	  3600}
};
#endif

#endif

