#pragma once
#ifndef CATA_SRC_FRAGMENT_CLOUD_H
#define CATA_SRC_FRAGMENT_CLOUD_H

enum class quadrant;

float shrapnel_calc( const float &initial,
                     const float &cloud,
                     const int &distance );
bool shrapnel_check( const float &cloud, const float &intensity );
void update_fragment_cloud( float &update, const float &new_value, quadrant );
float accumulate_fragment_cloud( const float &cumulative_cloud,
                                 const float &current_cloud,
                                 const int &distance );

#endif // CATA_SRC_FRAGMENT_CLOUD_H
