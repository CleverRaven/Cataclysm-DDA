#pragma once
#ifndef CATA_SRC_GAME_CONSTANTS_H
#define CATA_SRC_GAME_CONSTANTS_H

#include <map>
#include <string>
#include "units.h"

// Fixed window sizes.
static constexpr int EVEN_MINIMUM_TERM_WIDTH = 80;
static constexpr int EVEN_MINIMUM_TERM_HEIGHT = 24;
static constexpr int HP_HEIGHT = 14;
static constexpr int HP_WIDTH = 7;
static constexpr int MINIMAP_HEIGHT = 7;
static constexpr int MINIMAP_WIDTH = MINIMAP_HEIGHT;
static constexpr int MONINFO_HEIGHT = 12;
static constexpr int MONINFO_WIDTH = 48;
static constexpr int MESSAGES_HEIGHT = 8;
static constexpr int MESSAGES_WIDTH = 48;
static constexpr int LOCATION_HEIGHT = 1;
static constexpr int LOCATION_WIDTH = 48;
static constexpr int STATUS_HEIGHT = 4;
static constexpr int STATUS_WIDTH = 55;

static constexpr int BLINK_SPEED = 300;
static constexpr int EXPLOSION_MULTIPLIER = 7;

// Really just a sanity check for functions not tested beyond this. in theory 4096 works (`InvletInvlet).
static constexpr int MAX_ITEM_IN_SQUARE = 4096;
// no reason to differ.
static constexpr int MAX_ITEM_IN_VEHICLE_STORAGE = MAX_ITEM_IN_SQUARE;
// only can wear a maximum of two of any type of clothing.
static constexpr int MAX_WORN_PER_TYPE = 2;

static constexpr int MAPSIZE = 11;
static constexpr int HALF_MAPSIZE = static_cast<int>( MAPSIZE / 2 );

// SEEX/SEEY define the size of a nonant, or grid.
// All map segments will need to be at least this wide.
static constexpr int SEEX = 12;
static constexpr int SEEY = SEEX;

static constexpr int MAPSIZE_X = SEEX * MAPSIZE;
static constexpr int MAPSIZE_Y = SEEY * MAPSIZE;

static constexpr int HALF_MAPSIZE_X = SEEX * HALF_MAPSIZE;
static constexpr int HALF_MAPSIZE_Y = SEEY * HALF_MAPSIZE;

static constexpr int MAX_VIEW_DISTANCE = SEEX * HALF_MAPSIZE;

/**
 * Size of the overmap. This is the number of overmap terrain tiles per dimension in one overmap,
 * it's just like SEEX/SEEY for submaps.
*/
static constexpr int OMAPX = 180;
static constexpr int OMAPY = OMAPX;

// Size of a square unit of terrain saved to a directory.
static constexpr int SEG_SIZE = 32;

// Size of a square unit of tile memory saved in a single file, in mm_submaps.
static constexpr int MM_REG_SIZE = 8;

/**
 * Items on the map with at most this distance to the player are considered available for crafting,
 * see inventory::form_from_map
*/
static constexpr int PICKUP_RANGE = 6;

// Number of z-levels below 0 (not including 0).
static constexpr int OVERMAP_DEPTH = 10;
// Number of z-levels above 0 (not including 0).
static constexpr int OVERMAP_HEIGHT = 10;
// Total number of z-levels.
static constexpr int OVERMAP_LAYERS = 1 + OVERMAP_DEPTH + OVERMAP_HEIGHT;

// Maximum move cost when handling an item.
static constexpr int MAX_HANDLING_COST = 400;
// Move cost of accessing an item in inventory.
static constexpr int INVENTORY_HANDLING_PENALTY = 100;
// Move cost of accessing an item lying on the map. TODO: Less if player is crouching.
static constexpr int MAP_HANDLING_PENALTY = 80;
// Move cost of accessing an item lying on a vehicle.
static constexpr int VEHICLE_HANDLING_PENALTY = 80;

// Amount by which to charge an item for each unit of plutonium cell.
static constexpr int PLUTONIUM_CHARGES = 500;

// Temperature constants.
namespace temperatures
{
// temperature at which something starts is considered HOT.
constexpr units::temperature hot = units::from_fahrenheit( 100 ); // ~ 38 Celsius

// the "normal" temperature midpoint between cold and hot.
constexpr units::temperature normal = units::from_fahrenheit( 70 ); // ~ 21 Celsius

// Temperature inside an active fridge in Fahrenheit.
constexpr units::temperature fridge = units::from_fahrenheit( 37 ); // ~ 2.7 Celsius

// Temperature at which things are considered "cold".
constexpr units::temperature cold = units::from_fahrenheit( 40 ); // ~4.4 C

// Temperature inside an active freezer in Fahrenheit.
constexpr units::temperature freezer = units::from_celsius( -5 ); // -5 Celsius

// Temperature in which water freezes.
constexpr units::temperature freezing = units::from_celsius( 0 ); // 0 Celsius

// Temperature in which water boils.
constexpr units::temperature boiling = units::from_celsius( 100 ); // 100 Celsius
} // namespace temperatures

// Slowest speed at which a gun can be aimed.
static constexpr int MAX_AIM_COST = 10;

// Maximum (effective) level for a skill.
static constexpr int MAX_SKILL = 10;

// Maximum (effective) level for a stat.
static constexpr int MAX_STAT = 14;

// Maximum range at which ranged attacks can be executed.
static constexpr int RANGE_HARD_CAP = 60;

// Accuracy levels which a shots tangent must be below.
constexpr double accuracy_headshot = 0.1;
constexpr double accuracy_critical = 0.2;
constexpr double accuracy_goodhit  = 0.5;
constexpr double accuracy_standard = 0.8;
constexpr double accuracy_grazing  = 1.0;

// The maximum level recoil will ever reach.
// This corresponds to the level of accuracy of a "snap" or "hip" shot.
constexpr double MAX_RECOIL = 3000;

// Minimum item damage output of relevant type to allow using with relevant weapon skill.
static constexpr int MELEE_STAT = 5;

// Effective lower bound to combat skill levels when CQB bionic is active.
static constexpr int BIO_CQB_LEVEL = 5;

// Minimum size of a horde to show up on the minimap.
static constexpr int HORDE_VISIBILITY_SIZE = 3;

/**
 * Average annual temperature in F used for climate, weather and temperature calculation.
 * Average New England temperature = 43F/6C rounded to int.
*/
static constexpr units::temperature AVERAGE_ANNUAL_TEMPERATURE = units::from_fahrenheit( 43 );

/**
 * Base starting spring temperature in F used for climate, weather and temperature calculation.
 * New England base spring temperature = 65F/18C rounded to int.
*/
static constexpr units::temperature SPRING_TEMPERATURE = units::from_fahrenheit( 65 );

/**
 * Used to limit the random seed during noise calculation. A large value flattens the noise generator to zero.
 * Windows has a rand limit of 32768, other operating systems can have higher limits.
*/
constexpr int SIMPLEX_NOISE_RANDOM_SEED_LIMIT = 32768;

constexpr float MIN_MANIPULATOR_SCORE = 0.1f;
// the maximum penalty to movecost from a limb value
constexpr float MAX_MOVECOST_MODIFIER = 100.0f;

/**
 * activity levels, used for BMR.
 * these levels are normally used over the length of
 * days to weeks in order to calculate your total BMR
 * but we are making it more granular to be able to have
 * variable activity levels.
 * as such, when determining your activity level
 * in the json, think about what it would be if you
 * did this activity for a longer period of time.
*/
constexpr float SLEEP_EXERCISE = 0.85f;
constexpr float NO_EXERCISE = 1.0f;
constexpr float LIGHT_EXERCISE = 2.0f;
constexpr float MODERATE_EXERCISE = 4.0f;
constexpr float BRISK_EXERCISE = 6.0f;
constexpr float ACTIVE_EXERCISE = 8.0f;
constexpr float EXTRA_EXERCISE = 10.0f;

const std::map<std::string, float> activity_levels_map = {
    { "SLEEP_EXERCISE", SLEEP_EXERCISE },
    { "NO_EXERCISE", NO_EXERCISE },
    { "LIGHT_EXERCISE", LIGHT_EXERCISE },
    { "MODERATE_EXERCISE", MODERATE_EXERCISE },
    { "BRISK_EXERCISE", BRISK_EXERCISE },
    { "ACTIVE_EXERCISE", ACTIVE_EXERCISE },
    { "EXTRA_EXERCISE", EXTRA_EXERCISE }
};

const std::map<float, std::string> activity_levels_str_map = {
    { SLEEP_EXERCISE, "SLEEP_EXERCISE" },
    { NO_EXERCISE, "NO_EXERCISE" },
    { LIGHT_EXERCISE, "LIGHT_EXERCISE" },
    { MODERATE_EXERCISE, "MODERATE_EXERCISE" },
    { BRISK_EXERCISE, "BRISK_EXERCISE" },
    { ACTIVE_EXERCISE, "ACTIVE_EXERCISE" },
    { EXTRA_EXERCISE, "EXTRA_EXERCISE" }
};

// these are the lower bounds of each of the weight classes.
namespace character_weight_category
{
constexpr float emaciated = 14.0f;
constexpr float underweight = 16.0f;
constexpr float normal = 18.5f;
constexpr float overweight = 25.0f;
constexpr float obese = 30.0f;
constexpr float very_obese = 35.0f;
constexpr float morbidly_obese = 40.0f;
} // namespace character_weight_category

// these are the lower bounds of each of the health classes.
namespace character_health_category
{
//horrible
constexpr int very_bad = -100;
constexpr int bad = -50;
constexpr int fine = -10;
constexpr int good = 10;
constexpr int very_good = 50;
constexpr int great = 100;
} // namespace character_health_category

#endif // CATA_SRC_GAME_CONSTANTS_H
