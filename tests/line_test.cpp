#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"

#include "rng.h"
#include "line.h"

#include "stdio.h"

// Extracted from http://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html
// And lightly updated for use with timespec instead of timeval.
/* Subtract the `struct timespec' values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0. */
int
timespec_subtract( struct timespec *result, struct timespec *x, struct timespec *y)
{
    /* Perform the carry for the later subtraction by updating y. */
    if (x->tv_nsec < y->tv_nsec) {
        int nsec = (y->tv_nsec - x->tv_nsec) / 1000000000 + 1;
        y->tv_nsec -= 1000000000 * nsec;
        y->tv_sec += nsec;
    }
    if (x->tv_nsec - y->tv_nsec > 1000000000) {
        int nsec = (x->tv_nsec - y->tv_nsec) / 1000000000;
        y->tv_nsec += 1000000000 * nsec;
        y->tv_sec -= nsec;
    }

    /* Compute the time remaining to wait.
       tv_nsec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_nsec = x->tv_nsec - y->tv_nsec;

    /* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
}

#define SGN(a) (((a)<0) ? -1 : 1)
// Compare all future line_to implementations to the canonical one.
std::vector <point> canonical_line_to(const int x1, const int y1, const int x2, const int y2, int t)
{
    std::vector<point> ret;
    int dx = x2 - x1;
    int dy = y2 - y1;
    int ax = abs(dx) << 1;
    int ay = abs(dy) << 1;
    int sx = SGN(dx);
    int sy = SGN(dy);
    if (dy == 0) {
        sy = 0;
    }
    if (dx == 0) {
        sx = 0;
    }
    point cur;
    cur.x = x1;
    cur.y = y1;

    int xmin = (x1 < x2 ? x1 : x2);
    int ymin = (y1 < y2 ? y1 : y2);
    int xmax = (x1 > x2 ? x1 : x2);
    int ymax = (y1 > y2 ? y1 : y2);

    xmin -= abs(dx);
    ymin -= abs(dy);
    xmax += abs(dx);
    ymax += abs(dy);

    if (ax == ay) {
        do {
            cur.y += sy;
            cur.x += sx;
            ret.push_back(cur);
        } while ((cur.x != x2 || cur.y != y2) &&
                 (cur.x >= xmin && cur.x <= xmax && cur.y >= ymin && cur.y <= ymax));
    } else if (ax > ay) {
        do {
            if (t > 0) {
                cur.y += sy;
                t -= ax;
            }
            cur.x += sx;
            t += ay;
            ret.push_back(cur);
        } while ((cur.x != x2 || cur.y != y2) &&
                 (cur.x >= xmin && cur.x <= xmax && cur.y >= ymin && cur.y <= ymax));
    } else {
        do {
            if (t > 0) {
                cur.x += sx;
                t -= ay;
            }
            cur.y += sy;
            t += ax;
            ret.push_back(cur);
        } while ((cur.x != x2 || cur.y != y2) &&
                 (cur.x >= xmin && cur.x <= xmax && cur.y >= ymin && cur.y <= ymax));
    }
    return ret;
}

#define RANDOM_TEST_NUM 1000
#define COORDINATE_RANGE 99

TEST_CASE("Test bounds for mapping x/y/z/ offsets to direction enum") {

  // Test the unit cube, which are the only values this function is valid for.
  REQUIRE( make_xyz_unit(-1, -1, -1) == ABOVENORTHWEST );
  REQUIRE( make_xyz_unit(-1, -1, 0) == NORTHWEST );
  REQUIRE( make_xyz_unit(-1, -1, 1) == BELOWNORTHWEST );
  REQUIRE( make_xyz_unit(0, -1, -1) == ABOVENORTH );
  REQUIRE( make_xyz_unit(0, -1, 0) == NORTH );
  REQUIRE( make_xyz_unit(0, -1, 2) == BELOWNORTH );
  REQUIRE( make_xyz_unit(1, -1, -1) == ABOVENORTHEAST );
  REQUIRE( make_xyz_unit(1, -1, 0) == NORTHEAST );
  REQUIRE( make_xyz_unit(1, -1, 1) == BELOWNORTHEAST );
  REQUIRE( make_xyz_unit(-1, 0, -1) == ABOVEWEST );
  REQUIRE( make_xyz_unit(-1, 0, 0) == WEST );
  REQUIRE( make_xyz_unit(-1, 0, 1) == BELOWWEST );
  REQUIRE( make_xyz_unit(0, 0, -1) == ABOVECENTER );
  REQUIRE( make_xyz_unit(0, 0, 0) == CENTER );
  REQUIRE( make_xyz_unit(0, 0, 1) == BELOWCENTER );
  REQUIRE( make_xyz_unit(1, 0, -1) == ABOVEEAST );
  REQUIRE( make_xyz_unit(1, 0, 0) == EAST );
  REQUIRE( make_xyz_unit(1, 0, 1) == BELOWEAST );
  REQUIRE( make_xyz_unit(-1, 1, -1) == ABOVESOUTHWEST );
  REQUIRE( make_xyz_unit(-1, 1, 0) == SOUTHWEST );
  REQUIRE( make_xyz_unit(-1, 1, 1) == BELOWSOUTHWEST );
  REQUIRE( make_xyz_unit(0, 1, -1) == ABOVESOUTH );
  REQUIRE( make_xyz_unit(0, 1, 0) == SOUTH );
  REQUIRE( make_xyz_unit(0, 1, 1) == BELOWSOUTH );
  REQUIRE( make_xyz_unit(1, 1, -1) == ABOVESOUTHEAST );
  REQUIRE( make_xyz_unit(1, 1, 0) == SOUTHEAST );
  REQUIRE( make_xyz_unit(1, 1, 1) == BELOWSOUTHEAST );
  
  // Test the unit square values at distance 1 and 2.
  // Test the multiples of 30deg at 60 squares.
  // Test 22 deg to either side of the cardinal directions.
  REQUIRE( make_xyz(-1, -1, -1) == ABOVENORTHWEST );
  REQUIRE( make_xyz(-2, -2, -2) == ABOVENORTHWEST );
  REQUIRE( make_xyz(-30, -60, -1) == ABOVENORTHWEST );
  REQUIRE( make_xyz(-60, -60, -1) == ABOVENORTHWEST );
  REQUIRE( make_xyz(-60, -30, -1) == ABOVENORTHWEST );
  REQUIRE( make_xyz(-1, -1, 0) == NORTHWEST );
  REQUIRE( make_xyz(-2, -2, 0) == NORTHWEST );
  REQUIRE( make_xyz(-60, -60, 0) == NORTHWEST );
  REQUIRE( make_xyz(-1, -1, 1) == BELOWNORTHWEST );
  REQUIRE( make_xyz(-2, -2, 2) == BELOWNORTHWEST );
  REQUIRE( make_xyz(-30, -60, 1) == BELOWNORTHWEST );
  REQUIRE( make_xyz(-60, -60, 1) == BELOWNORTHWEST );
  REQUIRE( make_xyz(-60, -30, 1) == BELOWNORTHWEST );
  REQUIRE( make_xyz(0, -1, -1) == ABOVENORTH );
  REQUIRE( make_xyz(0, -2, -2) == ABOVENORTH );
  REQUIRE( make_xyz(-22, -60, -1) == ABOVENORTH );
  REQUIRE( make_xyz(0, -60, -1) == ABOVENORTH );
  REQUIRE( make_xyz(22, -60, -1) == ABOVENORTH );
  REQUIRE( make_xyz(0, -1, 0) == NORTH );
  REQUIRE( make_xyz(0, -2, 0) == NORTH );
  REQUIRE( make_xyz(-22, -60, 0) == NORTH );
  REQUIRE( make_xyz(0, -60, 0) == NORTH );
  REQUIRE( make_xyz(22, -60, 0) == NORTH );
  REQUIRE( make_xyz(0, -1, 1) == BELOWNORTH );
  REQUIRE( make_xyz(0, -2, 2) == BELOWNORTH );
  REQUIRE( make_xyz(-22, -60, 1) == BELOWNORTH );
  REQUIRE( make_xyz(0, -60, 1) == BELOWNORTH );
  REQUIRE( make_xyz(22, -60, 1) == BELOWNORTH );
  REQUIRE( make_xyz(1, -1, -1) == ABOVENORTHEAST );
  REQUIRE( make_xyz(2, -2, -2) == ABOVENORTHEAST );
  REQUIRE( make_xyz(30, -60, -1) == ABOVENORTHEAST );
  REQUIRE( make_xyz(60, -60, -1) == ABOVENORTHEAST );
  REQUIRE( make_xyz(60, -30, -1) == ABOVENORTHEAST );
  REQUIRE( make_xyz(1, -1, 0) == NORTHEAST );
  REQUIRE( make_xyz(2, -2, 0) == NORTHEAST );
  REQUIRE( make_xyz(30, -60, 0) == NORTHEAST );
  REQUIRE( make_xyz(60, -60, 0) == NORTHEAST );
  REQUIRE( make_xyz(60, -30, 0) == NORTHEAST );
  REQUIRE( make_xyz(1, -1, 1) == BELOWNORTHEAST );
  REQUIRE( make_xyz(2, -2, 2) == BELOWNORTHEAST );
  REQUIRE( make_xyz(30, -60, 1) == BELOWNORTHEAST );
  REQUIRE( make_xyz(60, -60, 1) == BELOWNORTHEAST );
  REQUIRE( make_xyz(60, -30, 1) == BELOWNORTHEAST );

  REQUIRE( make_xyz(-1, 0, -1) == ABOVEWEST );
  REQUIRE( make_xyz(-2, 0, -2) == ABOVEWEST );
  REQUIRE( make_xyz(-60, -22, -1) == ABOVEWEST );
  REQUIRE( make_xyz(-60, 0, -1) == ABOVEWEST );
  REQUIRE( make_xyz(-60, 22, -1) == ABOVEWEST );
  REQUIRE( make_xyz(-1, 0, 0) == WEST );
  REQUIRE( make_xyz(-2, 0, 0) == WEST );
  REQUIRE( make_xyz(-60, -22, 0) == WEST );
  REQUIRE( make_xyz(-60, 0, 0) == WEST );
  REQUIRE( make_xyz(-60, 22, 0) == WEST );
  REQUIRE( make_xyz(-1, 0, 1) == BELOWWEST );
  REQUIRE( make_xyz(-2, 0, 2) == BELOWWEST );
  REQUIRE( make_xyz(-60, -22, 1) == BELOWWEST );
  REQUIRE( make_xyz(-60, 0, 1) == BELOWWEST );
  REQUIRE( make_xyz(-60, 22, 1) == BELOWWEST );
  REQUIRE( make_xyz(0, 0, -1) == ABOVECENTER );
  REQUIRE( make_xyz(0, 0, -2) == ABOVECENTER );
  REQUIRE( make_xyz(0, 0, 0) == CENTER );
  REQUIRE( make_xyz(0, 0, 1) == BELOWCENTER );
  REQUIRE( make_xyz(0, 0, 2) == BELOWCENTER );
  REQUIRE( make_xyz(1, 0, -1) == ABOVEEAST );
  REQUIRE( make_xyz(2, 0, -2) == ABOVEEAST );
  REQUIRE( make_xyz(60, -22, -1) == ABOVEEAST );
  REQUIRE( make_xyz(60, 0, -1) == ABOVEEAST );
  REQUIRE( make_xyz(60, 22, -1) == ABOVEEAST );
  REQUIRE( make_xyz(1, 0, 0) == EAST );
  REQUIRE( make_xyz(2, 0, 0) == EAST );
  REQUIRE( make_xyz(60, -22, 0) == EAST );
  REQUIRE( make_xyz(60, 0, 0) == EAST );
  REQUIRE( make_xyz(60, 22, 0) == EAST );
  REQUIRE( make_xyz(1, 0, 1) == BELOWEAST );
  REQUIRE( make_xyz(2, 0, 2) == BELOWEAST );
  REQUIRE( make_xyz(60, -22, 1) == BELOWEAST );
  REQUIRE( make_xyz(60, 0, 1) == BELOWEAST );
  REQUIRE( make_xyz(60, 22, 1) == BELOWEAST );

  REQUIRE( make_xyz(-1, 1, -1) == ABOVESOUTHWEST );
  REQUIRE( make_xyz(-2, 2, -2) == ABOVESOUTHWEST );
  REQUIRE( make_xyz(-30, 60, -1) == ABOVESOUTHWEST );
  REQUIRE( make_xyz(-60, 60, -1) == ABOVESOUTHWEST );
  REQUIRE( make_xyz(-60, 30, -1) == ABOVESOUTHWEST );
  REQUIRE( make_xyz(-1, 1, 0) == SOUTHWEST );
  REQUIRE( make_xyz(-2, 2, 0) == SOUTHWEST );
  REQUIRE( make_xyz(-30, 60, 0) == SOUTHWEST );
  REQUIRE( make_xyz(-60, 60, 0) == SOUTHWEST );
  REQUIRE( make_xyz(-60, 30, 0) == SOUTHWEST );
  REQUIRE( make_xyz(-1, 1, 1) == BELOWSOUTHWEST );
  REQUIRE( make_xyz(-2, 2, 2) == BELOWSOUTHWEST );
  REQUIRE( make_xyz(-30, 60, 1) == BELOWSOUTHWEST );
  REQUIRE( make_xyz(-60, 60, 1) == BELOWSOUTHWEST );
  REQUIRE( make_xyz(-60, 30, 1) == BELOWSOUTHWEST );
  REQUIRE( make_xyz(0, 1, -1) == ABOVESOUTH );
  REQUIRE( make_xyz(0, 2, -2) == ABOVESOUTH );
  REQUIRE( make_xyz(0, 60, -1) == ABOVESOUTH );
  REQUIRE( make_xyz(0, 1, 0) == SOUTH );
  REQUIRE( make_xyz(-22, 60, 0) == SOUTH );
  REQUIRE( make_xyz(0, 60, 0) == SOUTH );
  REQUIRE( make_xyz(22, 60, 0) == SOUTH );
  REQUIRE( make_xyz(0, 1, 1) == BELOWSOUTH );
  REQUIRE( make_xyz(0, 2, 2) == BELOWSOUTH );
  REQUIRE( make_xyz(-22, 60, 1) == BELOWSOUTH );
  REQUIRE( make_xyz(0, 60, 1) == BELOWSOUTH );
  REQUIRE( make_xyz(22, 60, 1) == BELOWSOUTH );
  REQUIRE( make_xyz(1, 1, -1) == ABOVESOUTHEAST );
  REQUIRE( make_xyz(2, 2, -2) == ABOVESOUTHEAST );
  REQUIRE( make_xyz(30, 60, -1) == ABOVESOUTHEAST );
  REQUIRE( make_xyz(60, 60, -1) == ABOVESOUTHEAST );
  REQUIRE( make_xyz(60, 30, -1) == ABOVESOUTHEAST );
  REQUIRE( make_xyz(1, 1, 0) == SOUTHEAST );
  REQUIRE( make_xyz(2, 2, 0) == SOUTHEAST );
  REQUIRE( make_xyz(30, 60, 0) == SOUTHEAST );
  REQUIRE( make_xyz(60, 60, 0) == SOUTHEAST );
  REQUIRE( make_xyz(60, 30, 0) == SOUTHEAST );
  REQUIRE( make_xyz(1, 1, 1) == BELOWSOUTHEAST );
  REQUIRE( make_xyz(2, 2, 2) == BELOWSOUTHEAST );
  REQUIRE( make_xyz(30, 60, 1) == BELOWSOUTHEAST );
  REQUIRE( make_xyz(60, 60, 1) == BELOWSOUTHEAST );
  REQUIRE( make_xyz(60, 30, 1) == BELOWSOUTHEAST );
}

TEST_CASE("Compare line_to() to canonical line_to()") {

 REQUIRE( trig_dist(0, 0, 0, 0) == 0 );
 REQUIRE( trig_dist(0, 0, 1, 0) == 1 );

 const int seed = time( NULL );
 srandom( seed );

 for( int i = 0; i < RANDOM_TEST_NUM; ++i ) {
     const int x1 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
     const int y1 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
     const int x2 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
     const int y2 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
     int t1 = 0;
     int t2 = 0;
     REQUIRE( line_to( x1, y1, x2, y2, t1 ) == canonical_line_to( x1, y1, x2, y2, t2 ) );
 }

 {
     const int x1 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
     const int y1 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
     const int x2 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
     const int y2 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
     int t1 = 0;
     int t2 = 0;
     long count1 = 0;
     struct timespec start1;
     struct timespec end1;
     clock_gettime( CLOCK_REALTIME, &start1 );
     #define PERFORMANCE_TEST_ITERATIONS 10000
     while( count1 < PERFORMANCE_TEST_ITERATIONS ) {
         line_to( x1, y1, x2, y2, t1 );
         count1++;
     }
     clock_gettime( CLOCK_REALTIME, &end1 );
     long count2 = 0;
     struct timespec start2;
     struct timespec end2;
     clock_gettime( CLOCK_REALTIME, &start2 );
     while( count2 < PERFORMANCE_TEST_ITERATIONS ) {
         canonical_line_to( x1, y1, x2, y2, t2 );
         count2++;
     }
     clock_gettime( CLOCK_REALTIME, &end2 );
     struct timespec diff1;
     struct timespec diff2;
     timespec_subtract( &diff1, &end1, &start1 );
     timespec_subtract( &diff2, &end2, &start2 );

     // TODO: display this better, I doubt sec.nsec is an accurate rendering,
     // or at least reliably so.
     printf( "line_to() executed %d times in %ld.%ld seconds.\n",
             PERFORMANCE_TEST_ITERATIONS, diff1.tv_sec, diff1.tv_nsec );
     printf( "canonical_line_to() executed %d times in %ld.%ld seconds.\n",
             PERFORMANCE_TEST_ITERATIONS, diff2.tv_sec, diff2.tv_nsec );
 }
}
