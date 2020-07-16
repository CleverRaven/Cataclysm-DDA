#pragma once
#ifndef CATA_SRC_GAME_CONSTANTS_H
#define CATA_SRC_GAME_CONSTANTS_H

#include "units.h"

// Fixed window sizes.
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

#define BLINK_SPEED 300
#define EXPLOSION_MULTIPLIER 7

// Really just a sanity check for functions not tested beyond this. in theory 4096 works (`InvletInvlet).
#define MAX_ITEM_IN_SQUARE 4096
// no reason to differ.
#define MAX_ITEM_IN_VEHICLE_STORAGE MAX_ITEM_IN_SQUARE
// only can wear a maximum of two of any type of clothing.
#define MAX_WORN_PER_TYPE 2

#define MAPSIZE 11
#define HALF_MAPSIZE static_cast<int>( MAPSIZE / 2 )

// SEEX/SEEY define the size of a nonant, or grid.
// All map segments will need to be at least this wide.
#define SEEX 12
#define SEEY SEEX

#define MAPSIZE_X (SEEX * MAPSIZE)
#define MAPSIZE_Y (SEEY * MAPSIZE)

#define HALF_MAPSIZE_X (SEEX * HALF_MAPSIZE)
#define HALF_MAPSIZE_Y (SEEY * HALF_MAPSIZE)

#define MAX_VIEW_DISTANCE ( SEEX * HALF_MAPSIZE )

/**
 * Size of the overmap. This is the number of overmap terrain tiles per dimension in one overmap,
 * it's just like SEEX/SEEY for submaps.
*/
#define OMAPX 180
#define OMAPY 180

// Size of a square unit of terrain saved to a directory.
#define SEG_SIZE 32

/**
 * Items on the map with at most this distance to the player are considered available for crafting,
 * see inventory::form_from_map
*/
#define PICKUP_RANGE 6

// Number of z-levels below 0 (not including 0).
#define OVERMAP_DEPTH 10
// Number of z-levels above 0 (not including 0).
#define OVERMAP_HEIGHT 10
// Total number of z-levels.
#define OVERMAP_LAYERS (1 + OVERMAP_DEPTH + OVERMAP_HEIGHT)

// Maximum move cost when handling an item.
#define MAX_HANDLING_COST 400
// Move cost of accessing an item in inventory.
#define INVENTORY_HANDLING_PENALTY 100
// Move cost of accessing an item lying on the map. TODO: Less if player is crouching.
#define MAP_HANDLING_PENALTY 80
// Move cost of accessing an item lying on a vehicle.
#define VEHICLE_HANDLING_PENALTY 80

// Amount by which to charge an item for each unit of plutonium cell.
#define PLUTONIUM_CHARGES 500

// Temperature constants.
namespace temperatures
{
// temperature at which something starts is considered HOT.
constexpr int hot = 100; // ~ 38 Celsius

// the "normal" temperature midpoint between cold and hot.
constexpr int normal = 70; // ~ 21 Celsius

// Temperature inside an active fridge in Fahrenheit.
constexpr int fridge = 37; // ~ 2.7 Celsius

// Temperature at which things are considered "cold".
constexpr int cold = 40; // ~4.4 C

// Temperature inside an active freezer in Fahrenheit.
constexpr int freezer = 23; // -5 Celsius

// Temperature in which water freezes in Fahrenheit.
constexpr int freezing = 32; // 0 Celsius
} // namespace temperatures

// Weight per level of LIFT/JACK tool quality.
#define TOOL_LIFT_FACTOR 500_kilogram // 500kg/level

// Cap JACK requirements to support arbitrarily large vehicles.
#define JACK_LIMIT 8500_kilogram // 8500kg ( 8.5 metric tonnes )

// Slowest speed at which a gun can be aimed.
#define MAX_AIM_COST 10

// Maximum (effective) level for a skill.
#define MAX_SKILL 10

// Maximum (effective) level for a stat.
#define MAX_STAT 20

// Maximum range at which ranged attacks can be executed.
#define RANGE_HARD_CAP 60

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
#define MELEE_STAT 5

// Effective lower bound to combat skill levels when CQB bionic is active.
#define BIO_CQB_LEVEL 5

// Minimum size of a horde to show up on the minimap.
#define HORDE_VISIBILITY_SIZE 3

/**
 * Average annual temperature in F used for climate, weather and temperature calculation.
 * Average New England temperature = 43F/6C rounded to int.
*/
#define AVERAGE_ANNUAL_TEMPERATURE 43

/**
 * Base starting spring temperature in F used for climate, weather and temperature calculation.
 * New England base spring temperature = 65F/18C rounded to int.
*/
#define SPRING_TEMPERATURE 65

/**
 * Used to limit the random seed during noise calculation. A large value flattens the noise generator to zero.
 * Windows has a rand limit of 32768, other operating systems can have higher limits.
*/
constexpr int SIMPLEX_NOISE_RANDOM_SEED_LIMIT = 32768;

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
constexpr float NO_EXERCISE = 1.2f;
constexpr float LIGHT_EXERCISE = 1.375f;
constexpr float MODERATE_EXERCISE = 1.55f;
constexpr float ACTIVE_EXERCISE = 1.725f;
constexpr float EXTRA_EXERCISE = 1.9f;

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

#endif // CATA_SRC_GAME_CONSTANTS_H
