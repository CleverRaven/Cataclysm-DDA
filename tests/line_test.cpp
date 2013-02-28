/* libtap doesn't extern C their headers, so we do it for them. */
extern "C" {
 #include "tap.h"
}

#include <string.h>
#include "line.h"

int main(void)
{
 plan_tests(1);

 ok1(trig_dist(0, 0, 0, 0) == 0);

 return exit_status();
}
