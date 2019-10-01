#include <cmath>
#include <algorithm>

#include "overmap_noise.h"
#include "simplexnoise.h"

namespace om_noise
{

float om_noise_layer_forest::noise_at( const point &local_omt_pos ) const
{
    const point p = global_omt_pos( local_omt_pos );
    float r = scaled_octave_noise_3d( 8, 0.5, 0.03, 0, 1, p.x, p.y, get_seed() );
    r = powf( r, 2 );

    float d = scaled_octave_noise_3d( 12, 0.5, 0.07, 0, 1, p.x, p.y, get_seed() );
    d = powf( d, 3 );

    return std::max( 0.0f, r - d * 0.5f );
}

float om_noise_layer_floodplain::noise_at( const point &local_omt_pos ) const
{
    const point p = global_omt_pos( local_omt_pos );
    float r = scaled_octave_noise_3d( 8, 0.5, 0.05, 0, 1, p.x, p.y, get_seed() );
    r = powf( r, 2 );
    return r;
}

float om_noise_layer_lake::noise_at( const point &local_omt_pos ) const
{
    const point p = global_omt_pos( local_omt_pos );
    float r = scaled_octave_noise_3d( 16, 0.5, 0.002, 0, 1, p.x, p.y, get_seed() );
    r = powf( r, 4 );
    return r;
}

} // namespace om_noise
