#ifndef FIRE_H
#define FIRE_H

struct fire_data {
    int fire_intensity;

    int smoke_produced;
    int fuel_produced;
};

struct mat_burn_data {
    // Made to protect other materials too
    bool immune = false;

    // Fire has this in volume chance of working
    // If 0, always works
    int chance_in_volume = 0;
    // Fuel produced per tick
    int fuel = 0;
    // Smoke produced per tick
    int smoke = 0;
    // Burned volume per tick
    int burn = 0;
};

#endif
