#pragma once
#ifndef CATA_SRC_FRAGMENT_CLOUD_H
#define CATA_SRC_FRAGMENT_CLOUD_H

enum class quadrant;
/*
 * fragment_cloud represents the density and velocity of fragments passing through a square.
 */
struct fragment_cloud {
    fragment_cloud() : velocity( 0.0 ), density( 0.0 ) {}
    fragment_cloud( float initial_value ) : velocity( initial_value ), density( initial_value ) {}
    fragment_cloud( float initial_velocity, float initial_density )
        : velocity( initial_velocity ), density( initial_density ) {
    }
    fragment_cloud &operator=( const float &value );
    bool operator==( const fragment_cloud &that );
    /* Velocity is in m/sec. */
    float velocity;
    /* Density is a fuzzy count of number of fragments per cubic meter (one square). */
    float density;
};
bool operator<( const fragment_cloud &us, const fragment_cloud &them );

fragment_cloud shrapnel_calc( const fragment_cloud &initial,
                              const fragment_cloud &cloud,
                              const int &distance );
bool shrapnel_check( const fragment_cloud &cloud, const fragment_cloud &intensity );
void update_fragment_cloud( fragment_cloud &update, const fragment_cloud &new_value, quadrant );
fragment_cloud accumulate_fragment_cloud( const fragment_cloud &cumulative_cloud,
        const fragment_cloud &current_cloud,
        const int &distance );

#endif // CATA_SRC_FRAGMENT_CLOUD_H
