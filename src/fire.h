#ifndef FIRE_H
#define FIRE_H

struct fire_data {
    int fire_intensity;

    float smoke_produced;
    float fuel_produced;
};

struct mat_burn_data {
    // Made to protect other materials too
    bool immune = false;

    // Fire has this in volume chance of working
    // If 0, always works
    int chance_in_volume = 0;
    // Fractions are rolled as probability (to add 1)
    // Fuel produced per tick
    float fuel = 0.0f;
    // Smoke produced per tick
    float smoke = 0.0f;
    // Burned volume per tick
    float burn = 0.0f;
};

#endif
