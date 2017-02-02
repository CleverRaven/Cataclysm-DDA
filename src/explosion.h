#pragma once
#ifndef EXPLOSION_H
#define EXPLOSION_H

using itype_id = std::string;

class JsonObject;

struct shrapnel_data {
    int count           = 0;
    int mass            = 10;
    // Percentage
    int recovery        = 0;
    itype_id drop       = "null";
};

struct explosion_data {
    float power             = -1.0f;
    float distance_factor   = 0.8f;
    bool fire               = false;
    shrapnel_data shrapnel;

    /** Returns the distance at which we have `ratio` of initial power. */
    float expected_range( float ratio ) const;
    /** Returns the expected power at a given distance from epicenter. */
    float power_at_range( float dist ) const;
};

shrapnel_data load_shrapnel_data( JsonObject &jo );
explosion_data load_explosion_data( JsonObject &jo );

#endif
