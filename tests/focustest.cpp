#include "catch/catch.hpp"
#include "avatar.h"
#include "effect.h"

TEST_CASE( "focus effect", "[effect][focus]" )
{
    avatar &dummy = get_avatar();
    dummy.set_focus( 50 );
    const efftype_id eff_focus( "focus" );
    int focus_before = dummy.focus_pool;
    dummy.add_effect( eff_focus, 1_hours );
    dummy.process_turn();
    CAPTURE( dummy.focus_pool );
    CHECK( dummy.has_effect( eff_focus ) );
    CHECK( dummy.focus_pool > focus_before );
}