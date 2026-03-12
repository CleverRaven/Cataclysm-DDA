#include "cata_generators.h"

#include <memory>
#include <random>

#include "point.h"
#include "rng.h"

class RandomPointGenerator final :
    public Catch::Generators::IGenerator<point>
{
    public:
        RandomPointGenerator( int low, int high ) :
            engine( rng_get_engine() ),
            dist( low, high ) {
            this->next();
        }

        const point &get() const override {
            return current_point;
        }

        bool next() override {
            current_point = point( dist( engine ), dist( engine ) );
            return true;
        }
    protected:
        cata_default_random_engine &engine;
        std::uniform_int_distribution<> dist;
        point current_point;
};

class RandomTripointGenerator final :
    public Catch::Generators::IGenerator<tripoint>
{
    public:
        RandomTripointGenerator( int low, int high, int zlow, int zhigh ) :
            engine( rng_get_engine() ),
            xy_dist( low, high ),
            z_dist( zlow, zhigh ) {
            this->next();
        }

        const tripoint &get() const override {
            return current_point;
        }

        bool next() override {
            current_point = tripoint( xy_dist( engine ), xy_dist( engine ), z_dist( engine ) );
            return true;
        }
    protected:
        cata_default_random_engine &engine;
        std::uniform_int_distribution<> xy_dist;
        std::uniform_int_distribution<> z_dist;
        tripoint current_point;
};

Catch::Generators::GeneratorWrapper<point> random_points( int low, int high )
{
    return Catch::Generators::GeneratorWrapper<point>(
               std::make_unique<RandomPointGenerator>( low, high ) );
}

Catch::Generators::GeneratorWrapper<tripoint> random_tripoints(
    int low, int high, int zlow, int zhigh )
{
    return Catch::Generators::GeneratorWrapper<tripoint>(
               std::make_unique<RandomTripointGenerator>( low, high, zlow, zhigh ) );
}
