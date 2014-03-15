#include <tap++/tap++.h>
using namespace TAP;

#include "line.h"

int main(int argc, char *argv[])
{
 plan(1);

 ok( trig_dist(0, 0, 0, 0) == 0 );

 return exit_status();
}
