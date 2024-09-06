#include "cata_catch.h"

// A dummy test you can use to force load the game. The game is always loaded if
// any test without the [nogame] tag is run (including this one), which also
// means this test should never have the [nogame] tag, and other tests should
// never have the [force_load_game] tag.
TEST_CASE( "force_load_game", "[force_load_game]" )
{
}
