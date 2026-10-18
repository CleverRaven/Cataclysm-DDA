#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "cata_catch.h"
#include "cata_utility.h"
#include "coordinates.h"
#include "inventory_ui.h"
#include "item.h"
#include "item_location.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "ret_val.h"
#include "type_id.h"

class Character;

static const itype_id itype_backpack( "backpack" );
static const itype_id itype_bag_plastic( "bag_plastic" );
static const itype_id itype_hammer( "hammer" );
static const itype_id itype_knife_hunting( "knife_hunting" );

namespace
{
class counting_preset : public inventory_selector_preset
{
    public:
        explicit counting_preset( std::function<bool( const item & )> shows ) :
            shows( std::move( shows ) ) {}
        bool is_shown( const item_location &loc ) const override {
            ++asked[loc.get_item()];
            return shows( *loc );
        }
        mutable std::map<const item *, int> asked;
    private:
        std::function<bool( const item & )> shows;
};

class layout_selector : public inventory_selector
{
    public:
        using inventory_selector::inventory_selector;
        using inventory_selector::get_all_columns;
};

std::vector<std::string> layout_of( const layout_selector &selector )
{
    std::vector<std::string> out;
    for( const inventory_column *col : selector.get_all_columns() ) {
        for( const inventory_entry *e : col->get_entries( []( const inventory_entry & en ) {
        return en.is_item();
        }, true ) ) {
            out.push_back( e->any_item()->typeId().str() + " top=" +
                           ( e->topmost_parent ? e->topmost_parent->typeId().str() : "none" ) +
                           " indent=" + std::to_string( e->indent ) +
                           " chevron=" + ( e->chevron ? "yes" : "no" ) );
        }
    }
    return out;
}

void check_asked_once( const counting_preset &preset )
{
    for( const auto &[it, n] : preset.asked ) {
        CAPTURE( it->typeId() );
        CHECK( n == 1 );
    }
}
} // namespace

TEST_CASE( "inventory_selector_asks_the_preset_once_per_item", "[inventory][ui]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms pos = u.pos_bub();

    GIVEN( "backpack with a hammer and a knife" ) {
        item pack( itype_backpack );
        REQUIRE( pack.put_in( item( itype_hammer ), pocket_type::CONTAINER ).success() );
        REQUIRE( pack.put_in( item( itype_knife_hunting ), pocket_type::CONTAINER ).success() );
        REQUIRE_FALSE( here.add_item( pos, pack ).is_null() );

        WHEN( "every item shown" ) {
            counting_preset preset( return_true<item> );
            layout_selector selector( u, preset );
            selector.add_map_items( pos );
            THEN( "all three listed, each asked once" ) {
                CHECK( selector.item_entry_count() == 3 );
                CHECK( preset.asked.size() == 3 );
                check_asked_once( preset );
                CHECK( layout_of( selector ) == std::vector<std::string> {
                    "backpack top=none indent=0 chevron=yes",
                    "knife_hunting top=backpack indent=2 chevron=no",
                    "hammer top=backpack indent=2 chevron=no"
                } );
            }
        }
        WHEN( "backpack is hidden" ) {
            counting_preset preset( []( const item & it ) {
                return it.typeId() != itype_backpack;
            } );
            layout_selector selector( u, preset );
            selector.add_map_items( pos );
            THEN( "only contents listed, each item asked once" ) {
                CHECK( selector.item_entry_count() == 2 );
                check_asked_once( preset );
                CHECK( layout_of( selector ) == std::vector<std::string> {
                    "knife_hunting top=none indent=0 chevron=no",
                    "hammer top=none indent=0 chevron=no"
                } );
            }
        }
    }
    GIVEN( "backpack with plastic bag with hammer" ) {
        item bag( itype_bag_plastic );
        REQUIRE( bag.put_in( item( itype_hammer ), pocket_type::CONTAINER ).success() );
        item pack( itype_backpack );
        REQUIRE( pack.put_in( bag, pocket_type::CONTAINER ).success() );
        REQUIRE_FALSE( here.add_item( pos, pack ).is_null() );

        WHEN( "plastic bag in the middle hidden" ) {
            counting_preset preset( []( const item & it ) {
                return it.typeId() != itype_bag_plastic;
            } );
            layout_selector selector( u, preset );
            selector.add_map_items( pos );
            THEN( "backpack and hammer listed, each item asked once" ) {
                CHECK( selector.item_entry_count() == 2 );
                check_asked_once( preset );
                CHECK( layout_of( selector ) == std::vector<std::string> {
                    "backpack top=none indent=0 chevron=yes",
                    "hammer top=none indent=2 chevron=no"
                } );
            }
        }
    }
}
