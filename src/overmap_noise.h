#pragma once
#ifndef CATA_SRC_OVERMAP_NOISE_H
#define CATA_SRC_OVERMAP_NOISE_H

#include "game_constants.h"
#include "point.h"

namespace om_noise
{

/**
 * Abstract base class for generating noise for usage in overmap generation.
 * Subclass it and implement noise_at.
 */
class om_noise_layer
{
    public:
        /**
         * Noise value at the provided overmap terrain location.
         * @param omt_local point location in overmap terrain local coordinates.
         */
        virtual float noise_at( const point &omt_local ) const = 0;
        virtual ~om_noise_layer() = default;
    protected:
        /**
         * Providing the global base point of the overmap and a common seed across
         * overmaps will ensure that the noise is continuous across overmap boundaries.
         * @param global_base_point the (0, 0) corner of the overmap in global coordinates.
         * @param seed the seed to use for seeding the noise--conventionally the game's seed.
         */
        om_noise_layer( const point &global_base_point, const unsigned seed ) :
            om_global_base_point( global_base_point ),
            // Narrowing conversion into float, as the noise functions we use only accept floats.
            seed( seed % SIMPLEX_NOISE_RANDOM_SEED_LIMIT ) {
        }

        point global_omt_pos( const point &local_omt_pos ) const {
            return om_global_base_point + local_omt_pos;
        }

        float get_seed() const {
            return seed;
        }

    private:
        point om_global_base_point;
        float seed;
};

class om_noise_layer_forest : public om_noise_layer
{
    public:
        om_noise_layer_forest( const point &global_base_point, unsigned seed )
            : om_noise_layer( global_base_point, seed ) {
        }

        float noise_at( const point &local_omt_pos ) const override;
};

class om_noise_layer_floodplain : public om_noise_layer
{
    public:
        om_noise_layer_floodplain( const point &global_base_point, unsigned seed )
            : om_noise_layer( global_base_point, seed ) {
        }

        float noise_at( const point &local_omt_pos ) const override;
};

class om_noise_layer_lake : public om_noise_layer
{
    public:
        om_noise_layer_lake( const point &global_base_point, unsigned seed )
            : om_noise_layer( global_base_point, seed ) {
        }

        float noise_at( const point &local_omt_pos ) const override;
};

} // namespace om_noise

#endif // CATA_SRC_OVERMAP_NOISE_H
