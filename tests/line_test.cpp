#include <tap++/tap++.h>
using namespace TAP;

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

int main(int, char *[])
{
    (void)argc;
    (void)argv;
 plan( 2 + RANDOM_TEST_NUM );

 ok( trig_dist(0, 0, 0, 0) == 0 );
 ok( trig_dist(0, 0, 1, 0) == 1 );

 const int seed = time( NULL );
 srandom( seed );
 char test_message[100];

 for( int i = 0; i < RANDOM_TEST_NUM; ++i ) {
     const int x1 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
     const int y1 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
     const int x2 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
     const int y2 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
     int t1 = 0;
     int t2 = 0;
     snprintf( test_message, sizeof(test_message), "Line from %d, %d to %d, %d.",
               x1, y1, x2, y2 );
     ok( line_to( x1, y1, x2, y2, t1 ) == canonical_line_to( x1, y1, x2, y2, t2 ),
         test_message );
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
     #define PERFORMANCE_TEST_ITERATIONS 1000000
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

 return exit_status();
}
