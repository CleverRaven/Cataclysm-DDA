#ifndef _VEH_PHYSICS_H_
#define _VEH_PHYSICS_H_

#include "vehicle.h"
#include <vector>
#include <string>

// 0 - nothing, 1 - monster/player/npc, 2 - vehicle,
// 3 - thin_obstacle, 4 - bashable, 5 - destructible, 6 - other
enum veh_coll_type {
 veh_coll_nothing = 0,
 veh_coll_body,
 veh_coll_veh,
 veh_coll_thin_obstacle,
 veh_coll_bashable,
 veh_coll_destructable,
 veh_coll_other,

 num_veh_coll_types
};

struct veh_collision
{
    veh_coll_type type;       // What we're colliding with
    int x, y;                 // Coordinates of impact point
    int precalc_x, precalc_y; // Coordinates of vehicle part impacted
    float mass;               // For non-vehicles
    float elasticity;         // For non-vehicles
    int density;              // For non-vehicles
    void* target;             // Pointer to vehicle, mon, or npc
    std::string target_name;
    veh_collision() : type(veh_coll_nothing), x(0), y(0), mass(0.0f), elasticity(0.3f), density(15), target(NULL), target_name("") {};
};

class veh_physics
{
private:
    vehicle *veh;

    // Finds a single collision with vehicle, monster/NPC/player or terrain obstacle.
    // Return veh_collision, which has type, mass, and target.
    veh_collision get_point_collision (int x, int y);

    // Processes a single veh_collision, assigning damage and changing velocity/dir.
    void process_collision (veh_collision coll);

    // Calculate and deal damage to a single point from collition
    bool apply_damage_from_collision_to_point(int frame, veh_collision coll);

public:
    veh_physics();
    void init( vehicle *init_vehicle = NULL );

// Used in calculate_forces and div physical properties resulting from generating thrust
// These values are used when accelerating/braking and in the examine screen
    int mass;                        //  i| Copied from veh->cached_mass
    int eng_pwr_cur;                 //  i| Current power from engines, in 0.5HP
//    int eng_pwr_fueled;              //  i| Power if all engines had fuel, in 0.5HP
    int eng_pwr_max;                 //  i| Power if at max hp and fuelled, in 0.5HP
    int eng_pwr_alt;                 //  i| Power drain from alternators, in 0.5HP
    int v_dir;                       // st| -1 if going backwards, 1 if forwards
    bool is_thrusting;               //  f| If engine is used to provide thrust
    bool valid_wheel_config;         //  i| Cached, if it has enough wheels to acc/brake
    double distance_traveled;        // mt| How far the vehicle traveled, in m
    double downforce_newton;         //  f| Force pushing vehicle towards the ground 
    double drag_newton;              //  f| Force slowing vehicle from wind res
    double engine_newton;            //  f| Force used to actually drive wheels
    double engine_newton_avail;      // mt| How much the engine can work
    double engine_watt_avail;        //  i| Available power
    double engine_watt_average;      // mt| How much power engine actually used
    double ground_res_coeff;         //  d| Rolling resistance and debris
    double ground_res_newton;        //  f| Force slowing vehicle from rolling/debris
    double kinetic_end;              // mt| Initial kinetic energy, in J
    double kinetic_start;            // st| Kinetic energy after calculations, in J
    double newton_total;             //  f| Sum of all the forces working on the vehicle
    double newton_average;           // mt| Average sum of all the forces
//    double rolling_res_coeff;        // Wheel resistance
    double time_taken;               // et| Time it took to move, in s
    double tire_friction_coeff;      //  d| How well the tires grip the ground
    double user_applied_newton;      //  f| Force actually driving wheels
    double velocity_average;         // mt| m/s
    double velocity_end;             // mt| m/s
    double velocity_start;           // st| m/s
    double wheel_newton_avail;       //  f| Grip available for thrust/steering
    double wheel_newton_max;         //  f| Total grip

    std::string first_collision_name;// Used for collisions

    // Calculate the forces currently acting on the vehicle and store them in struct vf
    // v is velocity to calculate at. returns net force of newtons acting on vehicle
    double calculate_forces( double v, double fdir );

    // Sum up the forces working over a single tile where fdir=engine thrust/braking,
    // reiterate with different time values until the correct time is found,
    // store them in vf, and return calculated vehicle velocity
    double calculate_time_to_move_one_tile( double fdir );

    // Calculate factors and coefficiencies from current tiles/temperature/grip etc,
    // store them in vf, and then figure out what parameters (time/thrust) to use
    // in order to move exactly 1 tile length and end up at the desired velocity.
    // Also used by veh_interact to display stuff
    double calculate_movement( double engine_output, double target_speed = 0.0 );

    // Handles all collisions between this vehicle and other stuff at the given delta.
    bool collision( int dx, int dy, bool just_detect );

    // Move through current maptile. Handle friction/wheel damage etc. Called by map. Return false if failed.
    bool drive_one_tile();

    // Process the trap beneath
    void handle_trap (int x, int y, int part);

};

#endif
