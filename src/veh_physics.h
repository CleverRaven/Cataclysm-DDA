#ifndef VEH_PHYSICS_H
#define VEH_PHYSICS_H

#include "vehicle.h"
#include <vector>
#include <string>

class veh_physics
{
private:
    vehicle *veh;

public:
    veh_physics(vehicle *v) : veh(v) {
        mass = veh->total_mass();
        eng_pwr_cur = veh->total_power();
        eng_pwr_max = veh->total_power(false);
        engine_watt_avail = veh->engine_on ? double(veh->power_to_epower(eng_pwr_cur)) : 0.0;
        valid_wheel_config = veh->valid_wheel_config(); /* Cache this */
        tire_friction_coeff = 0.7;
        ground_res_coeff = 0.01;
    }

    /**
     * These values are used when accelerating/braking and in the examine screen
     * The first letters of the comments shows where the value is used:
     * i=internal, basic properties of vehicle, copied from vehicle, setup in constructor
     * s=starting conditions, state of vehicle before calculations
     * d=changed externally, changes depending on terrain driven over
     * f=calculate_forces()
     * t=calculate_time_to_move_one_tile()
     * m=calculate_movement()
     * e=estimated value, returned to external callers for use there
     */
    int mass;                        /*   i| Copied from veh->cached_mass */
    int eng_pwr_cur;                 /*   i| Current power from engines, in 0.5HP */
    int eng_pwr_max;                 /*   i| Power if at max hp and fuelled, in 0.5HP */
    int eng_pwr_alt;                 /*   i| Power drain from alternators, in 0.5HP */
    int v_dir;                       /*  st| -1 if going backwards, 1 if forwards */
    bool is_thrusting;               /*   f| If engine is used to provide thrust */
    bool valid_wheel_config;         /*   i| Cached, if it has enough wheels to acc/brake */
    double distance_traveled;        /*  mt| How far the vehicle traveled, in m */
    double downforce_newton;         /*   f| Force pushing vehicle towards the ground  */
    double drag_newton;              /*   f| Force slowing vehicle from wind res */
    double engine_newton;            /*   f| Force used to actually drive wheels */
    double engine_newton_avail;      /*  mt| How much the engine can work */
    double engine_watt_avail;        /*   i| Available power */
    double engine_watt_average;      /*  mt| How much power engine actually used */
    double ground_res_coeff;         /*   d| Rolling resistance and debris */
    double ground_res_newton;        /*   f| Force slowing vehicle from rolling/debris */
    double kinetic_end;              /*  mt| Initial kinetic energy, in J */
    double kinetic_start;            /*  st| Kinetic energy after calculations, in J */
    double newton_total;             /*   f| Sum of all the forces working on the vehicle */
    double newton_average;           /*  mt| Average sum of all the forces */
    double time_taken;               /*  et| Time it took to move, in s */
    double tire_friction_coeff;      /*   d| How well the tires grip the ground */
    double user_applied_newton;      /*   f| Force actually driving wheels */
    double velocity_average;         /*  mt| m/s */
    double velocity_end;             /* emt| m/s */
    double velocity_start;           /*  st| m/s */
    double wheel_newton_avail;       /*   f| Grip available for thrust/steering */
    double wheel_newton_max;         /*   f| Total grip */
    const double air_density = 1.204; /* in kg/m^3 */
    const double gravity = 9.80665;   /* in m/s^2 */

    /**
     * This function assumes flat ground and homes in on the speed at which the vehicle will
     * neither gain nor lose speed when using full power (ie top speed).
     * Used by veh_interact to display vehicle stats.
     * @param max_power Assume all engines are 100% health and fuelled
     * @return Max speed reached in meters / second
     */
    double calculate_top_speed( bool max_power = false );

    /**
     * This function assumes flat ground and accelerates (from a full stop) as fast as
     * possible across multiple tiles (max 1000 tiles or 10 secs) until target_speed is reached.
     * Used by veh_interact to display vehicle stats.
     * @param target_speed Target speed to accelerate to in m/s
     * @param max_power Assume all engines are 100% health and fuelled
     * @return Time taken to reach target_speed, if negative it's speed reached after 10 secs instead
     */
     double calculate_acceleration( double target_speed, bool max_power = false );

    /**
     * Cruise control mode tries to adjust engine_output so that
     * target_speed is reached after driving across the current tile.
     * This is non-trivial as it depends on some 20 different variables.
     * @param target_speed Cruise speed we're trying to reach
     * @return Time taken to drive across this tile
     */
    double calculate_cruise_control( double target_speed );

    /**
     * Calculate factors and coefficiencies from current tiles/speed/grip etc,
     * store them in this class, and then figure out what parameters (time/thrust) to use
     * in order to move exactly 1 tile length and end up at the desired velocity.
     * Normal thrusting mode ignores target_speed and calculates movement across a single tile
     * when the driver accelerates or brakes at engine_output [-1, 1],
     * @return Time taken to drive across this tile
     */
    double calculate_time_to_move_one_tile( double fdir );

    /**
     * Calculate the forces currently acting on the vehicle and store them in this class.
     * @param v Starting velocity to calculate at
     * @param fdir Engine force to use [-1.0, 1.0] 
     * @return Net force of newtons acting on vehicle
     */
    double calculate_forces( double v, double fdir );

};

#endif
