#ifndef FIRE_H
#define FIRE_H

struct fire_data {
    int fire_intensity;

    int smoke_produced;
    int fuel_produced;
};

struct mat_burn_data {
    // Made to protect other materials too
    bool immune;

    // Fire has this in volume chance of working
    // If 0, always works
    int chance_in_volume;
    // Fuel produced per tick
    int fuel;
    // Smoke produced per tick
    int smoke;
    // Burned volume per tick
    int burn;
};

#endif
