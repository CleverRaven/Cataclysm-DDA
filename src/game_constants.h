#ifndef GAME_CONSTANTS_H
#define GAME_CONSTANTS_H

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Fixed window sizes
#define HP_HEIGHT 14
#define HP_WIDTH 7
#define MINIMAP_HEIGHT 7
#define MINIMAP_WIDTH 7
#define MONINFO_HEIGHT 12
#define MONINFO_WIDTH 48
#define MESSAGES_HEIGHT 8
#define MESSAGES_WIDTH 48
#define LOCATION_HEIGHT 1
#define LOCATION_WIDTH 48
#define STATUS_HEIGHT 4
#define STATUS_WIDTH 55

#define LONG_RANGE 10
#define BLINK_SPEED 300
#define EXPLOSION_MULTIPLIER 7

#define MAX_ITEM_IN_SQUARE 4096 // really just a sanity check for functions not tested beyond this. in theory 4096 works (`InvletInvlet)
#define MAX_VOLUME_IN_SQUARE 4000 // 6.25 dead bears is enough for everybody!
#define MAX_ITEM_IN_VEHICLE_STORAGE MAX_ITEM_IN_SQUARE // no reason to differ

// vital thresholds and messages
const int HUNGER_FREQUENCY = 20;
const int HUNGER_MINOR = 3000;
const int HUNGER_MAJOR = 4000;
const int HUNGER_CRITICAL = 5000;
const int HUNGER_DEATH = 6000;

#define HUNGER_MINOR_MSG "Your stomach feels so empty..."
#define HUNGER_MAJOR_MSG "You are STARVING!"
#define HUNGER_CRITICAL_MSG "Food..."
#define HUNGER_DEATH_MSG "You have starved to death."

#define HUNGER_MALE_MEMORIAL_MSG "Died of starvation"
#define HUNGER_FEMALE_MEMORIAL_MSG "Died of starvation"

const int THIRST_FREQUENCY = 20;
const int THIRST_MINOR = 600;
const int THIRST_MAJOR = 800;
const int THIRST_CRITICAL = 1000;
const int THIRST_DEATH = 1200;

#define THIRST_MINOR_MSG "Your mouth feels so dry..."
#define THIRST_MAJOR_MSG "You are THIRSTY!"
#define THIRST_CRITICAL_MSG "Even your eyes feel dry..."
#define THIRST_DEATH_MSG "You have died of dehydration."

#define THIRST_MALE_MEMORIAL_MSG "Died of thirst."
#define THIRST_FEMALE_MEMORIAL_MSG "Died of thirst."

const int FATIGUE_FREQUENCY = 50;
const int FATIGUE_TRIVIAL = 383;
const int FATIGUE_MINOR = 575;
const int FATIGUE_MAJOR = 600;
const int FATIGUE_CRITICAL = 700;
const int FATIGUE_DIRE = 800;
const int FATIGUE_SLEEP = 1000;

#define FATIGUE_TRIVIAL_MSG "*yawn* You should really get some sleep."
#define FATIGUE_MINOR_MSG "How much longer until bedtime?"
#define FATIGUE_MAJOR_MSG "You're too tired to stop yawning."
#define FATIGUE_CRITICAL_MSG "You feel like you haven't slept in days."
#define FATIGUE_DIRE_MSG "Anywhere would be a good place to sleep..."
#define FATIGUE_SLEEP_MSG "Survivor sleep now."

#define FATIGUE_MALE_MEMORIAL_MSG "Succumbed to lack of sleep"
#define FATIGUE_FEMALE_MEMORIAL_MSG "Succumbed to lack of sleep"

#endif
