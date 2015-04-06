#include <tap++/tap++.h>
using namespace TAP;

#include "map.h"

#include <chrono>
#include <random>
#include "stdio.h"

// Constants setting the ratio of set to unset tiles.
constexpr int NUMERATOR = 1;
constexpr int DENOMINATOR = 10;

// The width and height of the area being checked.
constexpr int DIMENSION = 121;

void oldCastLight( bool (&output_cache)[MAPSIZE*SEEX][MAPSIZE*SEEY],
                   const float (&input_array)[MAPSIZE*SEEX][MAPSIZE*SEEY],
                   const int xx, const int xy, const int yx, const int yy,
                   const int offsetX, const int offsetY, const int offsetDistance,
                   const int row, float start, const float end )
{
    float newStart = 0.0f;
    float radius = 60.0f - offsetDistance;
    if( start < end ) {
        return;
    }
    bool blocked = false;
    for( int distance = row; distance <= radius && !blocked; distance++ ) {
        int deltaY = -distance;
        for( int deltaX = -distance; deltaX <= 0; deltaX++ ) {
            int currentX = offsetX + deltaX * xx + deltaY * xy;
            int currentY = offsetY + deltaX * yx + deltaY * yy;
            float leftSlope = (deltaX - 0.5f) / (deltaY + 0.5f);
            float rightSlope = (deltaX + 0.5f) / (deltaY - 0.5f);

            if( !(currentX >= 0 && currentY >= 0 && currentX < SEEX * my_MAPSIZE &&
                  currentY < SEEY * my_MAPSIZE) || start < rightSlope ) {
                continue;
            } else if( end > leftSlope ) {
                break;
            }

            //check if it's within the visible area and mark visible if so
            if( rl_dist(0, 0, deltaX, deltaY) <= radius ) {
                /*
                  float bright = (float) (1 - (rStrat.radius(deltaX, deltaY) / radius));
                  lightMap[currentX][currentY] = bright;
                */
                output_cache[currentX][currentY] = true;
            }

            if( blocked ) {
                //previous cell was a blocking one
                if( input_array[currentX][currentY] == LIGHT_TRANSPARENCY_SOLID ) {
                    //hit a wall
                    newStart = rightSlope;
                    continue;
                } else {
                    blocked = false;
                    start = newStart;
                }
            } else {
                if( input_array[currentX][currentY] == LIGHT_TRANSPARENCY_SOLID &&
                    distance < radius ) {
                    //hit a wall within sight line
                    blocked = true;
                    castLight(output_cache, input_array, xx, xy, yx, yy,
                              offsetX, offsetY, offsetDistance,  distance + 1, start, leftSlope );
                    newStart = rightSlope;
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    plan( 2 + RANDOM_TEST_NUM );

    // Construct a rng that produces integers in a range selected to provide the probability
    // we want, i.e. if we want 1/4 tiles to be set, produce numbers in the range 0-3,
    // with 0 indicating the bit is set.
    const unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator;
    std::uniform_int_distribution<unsigned int> distribution(0, DENOMINATOR);
    auto rng = std::bind ( distribution, generator );

    bool seen_squares_control[MAPSIZE*SEEX][MAPSIZE*SEEY] = {0};
    bool seen_squares_experiment[MAPSIZE*SEEX][MAPSIZE*SEEY] = {0};
    float transparency_cache[MAPSIZE*SEEX][MAPSIZE*SEEY] = {0};

    // Initialize the transparency value of each square to a random value.
    for( float &square : transparency_cache ) {
        if( rng() < NUMERATOR ) {
            square = 1.0f;
        }
    }

    map dummy;

    const int offsetX = 65;
    const int offsetY = 65;

    // First the control algorithm.
    oldCastLight( seen_squares_control, transparency_cache, 0, 1, 1, 0, offsetX, offsetY, 0 );
    oldCastLight( seen_squares_control, transparency_cache, 1, 0, 0, 1, offsetX, offsetY, 0 );

    oldCastLight( seen_squares_control, transparency_cache, 0, -1, 1, 0, offsetX, offsetY, 0 );
    oldCastLight( seen_squares_control, transparency_cache, -1, 0, 0, 1, offsetX, offsetY, 0 );

    oldCastLight( seen_squares_control, transparency_cache, 0, 1, -1, 0, offsetX, offsetY, 0 );
    oldCastLight( seen_squares_control, transparency_cache, 1, 0, 0, -1, offsetX, offsetY, 0 );

    oldCastLight( seen_squares_control, transparency_cache, 0, -1, -1, 0, offsetX, offsetY, 0 );
    oldCastLight( seen_squares_control, transparency_cache, -1, 0, 0, -1, offsetX, offsetY, 0 );

    // Then the experimental algorithm.
    dummy.castLight( seen_squares_experiment, transparency_cache, 0, 1, 1, 0, offsetX, offsetY, 0 );
    dummy.castLight( seen_squares_experiment, transparency_cache, 1, 0, 0, 1, offsetX, offsetY, 0 );

    dummy.castLight( seen_squares_experiment, transparency_cache, 0, -1, 1, 0, offsetX, offsetY, 0 );
    dummy.castLight( seen_squares_experiment, transparency_cache, -1, 0, 0, 1, offsetX, offsetY, 0 );

    dummy.castLight( seen_squares_experiment, transparency_cache, 0, 1, -1, 0, offsetX, offsetY, 0 );
    dummy.castLight( seen_squares_experiment, transparency_cache, 1, 0, 0, -1, offsetX, offsetY, 0 );

    dummy.castLight( seen_squares_experiment, transparency_cache, 0, -1, -1, 0, offsetX, offsetY, 0 );
    dummy.castLight( seen_squares_experiment, transparency_cache, -1, 0, 0, -1, offsetX, offsetY, 0 );

    bool passed = true;
    for( int x = 0; x < sizeof(seen_squares_control); ++x ) {
        for( int y = 0; y < sizeof(seen_squares_control); ++y ) {
            if( seen_squares_control != seen_squares_experiment ) {
                passed = false;
            }
        }
    }

    ok( passed );

    return exit_status();
}
