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
const HUNGER_FREQUENCY 20;
const HUNGER_MINOR 3000;
const HUNGER_MAJOR 4000;
const HUNGER_CRITICAL 5000;
const HUNGER_DEATH 6000;

const HUNGER_MINOR_MSG "Your stomach feels so empty...";
const HUNGER_MAJOR_MSG "You are STARVING!";
const HUNGER_CRITICAL_MSG "Food...";
const HUNGER_DEATH_MSG "You have starved to death.";

const HUNGER_MALE_MEMORIAL_MSG "Died of starvation";
const HUNGER_FEMALE_MEMORIAL_MSG "Died of starvation";

const THIRST_FREQUENCY 20;
const THIRST_MINOR 600;
const THIRST_MAJOR 800;
const THIRST_CRITICAL 1000;
const THIRST_DEATH 1200;

const THIRST_MINOR_MSG "Your mouth feels so dry...";
const THIRST_MAJOR_MSG "You are THIRSTY!";
const THIRST_CRITICAL_MSG "Even your eyes feel dry...";
const THIRST_DEATH_MSG "You have died of dehydration.";

const THIRST_MALE_MEMORIAL_MSG "Died of thirst.";
const THIRST_FEMALE_MEMORIAL_MSG "Died of thirst.";

const FATIGUE_FREQUENCY 50;
const FATIGUE_TRIVIAL 383;
const FATIGUE_MINOR 575;
const FATIGUE_MAJOR 600;
const FATIGUE_CRITICAL 700;
const FATIGUE_DIRE 800;
const FATIGUE_SLEEP 1000;

const FATIGUE_TRIVIAL_MSG "*yawn* You should really get some sleep.";
const FATIGUE_MINOR_MSG "How much longer until bedtime?";
const FATIGUE_MAJOR_MSG "You're too tired to stop yawning.";
const FATIGUE_CRITICAL_MSG "You feel like you haven't slept in days.";
const FATIGUE_DIRE_MSG "Anywhere would be a good place to sleep...";
const FATIGUE_SLEEP_MSG "Survivor sleep now.";

const FATIGUE_MALE_MEMORIAL_MSG "Succumbed to lack of sleep";
const FATIGUE_FEMALE_MEMORIAL_MSG "Succumbed to lack of sleep";

#endif
