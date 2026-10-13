#include <vector>

#include "avatar.h"
#include "cata_catch.h"
#include "item.h"
#include "item_location.h"
#include "pocket_type.h"
#include "ret_val.h"
#include "type_id.h"

static const itype_id itype_backpack( "backpack" );
static const itype_id itype_m1911( "m1911" );

TEST_CASE( "available_reloadables", "[ammo][pockets]" )
{
    avatar &guy = get_avatar();
    item_location m1911 = guy.i_add( item( itype_m1911 ) );
    auto reloadables = guy.find_reloadables();
    REQUIRE( reloadables.size() == 1 );
    REQUIRE( reloadables.front() == m1911 );
    CHECK( reloadables.front().obtain_cost( guy ) == 0 );
    m1911.remove_item();
    REQUIRE( !guy.has_weapon() );
    item_location pack = guy.i_add( item( itype_backpack ) );
    auto res = pack->put_in( item( itype_m1911 ), pocket_type::CONTAINER );
    CHECK( guy.get_wielded_item()->typeId() == itype_backpack );
    REQUIRE( res.success() );
    reloadables.clear();
    reloadables = guy.find_reloadables();
    REQUIRE( reloadables.size() == 1 );
    CHECK( reloadables.front()->typeId() == itype_m1911 );
    m1911 = reloadables.front();
    REQUIRE( m1911.has_parent() );
    CHECK( m1911.parent_item() == pack );
    CHECK( m1911.obtain_cost( guy ) == 322 );
}
