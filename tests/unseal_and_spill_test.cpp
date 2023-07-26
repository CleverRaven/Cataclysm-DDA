#include <iosfwd>
#include <list>
#include <new>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include "avatar.h"
#include "cached_options.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "character.h"
#include "colony.h"
#include "contents_change_handler.h"
#include "item.h"
#include "item_location.h"
#include "item_pocket.h"
#include "item_stack.h"
#include "map.h"
#include "map_helpers.h"
#include "map_selector.h"
#include "player_helpers.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "vpart_position.h"

// *INDENT-OFF*
static const itype_id itype_test_liquid_1ml( "test_liquid_1ml" );
static const itype_id itype_test_rag("test_rag");
static const itype_id itype_test_restricted_container_holder( "test_restricted_container_holder" );
static const itype_id itype_test_solid_1ml( "test_solid_1ml" );
static const itype_id itype_test_watertight_open_sealed_container_1L( "test_watertight_open_sealed_container_1L" );
static const itype_id itype_test_watertight_open_sealed_container_250ml( "test_watertight_open_sealed_container_250ml" );
static const itype_id itype_test_watertight_open_sealed_multipocket_container_2x1L( "test_watertight_open_sealed_multipocket_container_2x1L" );
static const itype_id itype_test_watertight_open_sealed_multipocket_container_2x250ml( "test_watertight_open_sealed_multipocket_container_2x250ml" );

static const vproto_id vehicle_prototype_test_cargo_space( "test_cargo_space" );
// *INDENT-ON*

namespace
{
template<typename ...Enums>
class enum_tuple
{
    private:
        using tuple_type = std::tuple<Enums...>;
        tuple_type enums;

        template<int ind>
        void increment() {
            using enum_t = typename std::tuple_element<ind, tuple_type>::type;
            enum_t &val = std::get<ind>( enums );
            if( val != enum_t::end ) {
                val = static_cast<enum_t>( static_cast<int>( val ) + 1 );
            }
            if( val == enum_t::end && ind > 0 ) {
                val = enum_t::begin;
                // *INDENT-OFF*
                increment<( ind > 0 ? ind - 1 : 0 )>();
                // *INDENT-ON*
            }
        }

    public:
        explicit enum_tuple( Enums ...enums )
            : enums( enums... ) {
        }

        template<typename Enum>
        Enum get() const {
            return std::get<Enum>( enums );
        }

        enum_tuple operator++() {
            // *INDENT-OFF*
            increment<std::tuple_size<tuple_type>::value - 1>();
            // *INDENT-ON*
            return *this;
        }

        bool operator==( const enum_tuple &rhs ) const {
            return enums == rhs.enums;
        }

        bool operator!=( const enum_tuple &rhs ) const {
            return !operator==( rhs );
        }

        static enum_tuple begin() {
            return enum_tuple( Enums::begin... );
        }

        static enum_tuple end() {
            return ++enum_tuple( static_cast<Enums>( static_cast<int>( Enums::end ) - 1 )... );
        }
};

enum class container_location : int {
    begin,
    inventory = begin,
    worn,
    wielded,
    vehicle,
    ground,
    end,
};

enum class scenario : int {
    begin,
    contained_liquid = begin,
    nested_contained_liquid,
    recursive_multi_pocket,
    end,
};

enum class player_action : int {
    begin,
    spill_all = begin,
    cancel_spill,
    end,
};

class test_scenario
{
    private:
        using enum_tuple_type = enum_tuple<container_location, scenario, player_action>;
        enum_tuple_type value;

    public:
        explicit test_scenario( const enum_tuple_type &value ) : value( value ) {
        }

        void run();

        test_scenario operator++() {
            ++value;
            return *this;
        }

        bool operator==( const test_scenario &rhs ) const {
            return value == rhs.value;
        }

        bool operator!=( const test_scenario &rhs ) const {
            return !operator==( rhs );
        }

        static test_scenario begin() {
            return test_scenario( enum_tuple_type::begin() );
        }

        static test_scenario end() {
            return test_scenario( enum_tuple_type::end() );
        }
};

void unseal_items_containing( contents_change_handler &handler, item_location &root,
                              const std::set<itype_id> &types )
{
    for( item *it : root->all_items_top( item_pocket::pocket_type::CONTAINER ) ) {
        if( it ) {
            item_location content( root, it );
            if( types.count( it->typeId() ) ) {
                handler.unseal_pocket_containing( content );
            }
            unseal_items_containing( handler, content, types );
        }
    }
}

struct initialization {
    itype_id id;
    bool fill_parent;
    bool seal;
    std::vector<initialization> contents;
};

item initialize( const initialization &init )
{
    item it( init.id );
    for( const initialization &content_init : init.contents ) {
        item content = initialize( content_init );
        if( content_init.fill_parent ) {
            REQUIRE( it.fill_with( content ) >= 1 );
        } else {
            ret_val<void> ret = it.put_in( content, item_pocket::pocket_type::CONTAINER );
            INFO( ret.str() );
            REQUIRE( ret.success() );
        }
    }
    if( init.seal ) {
        REQUIRE( it.seal() );
    }
    return it;
}

struct final_result {
    itype_id id;
    bool sealed;
    bool parent_pocket_sealed;
    std::vector<final_result> contents;
};

item *item_pointer( item *const it )
{
    return it;
}

item *item_pointer( item &it )
{
    return &it;
}

item *item_pointer( item_location it )
{
    return it.get_item();
}

template < typename Parent,
           std::enable_if_t < !std::is_same<std::decay_t<Parent>, item_location>::value, int > = 0 >
item_location container_from_parent( Parent && )
{
    return item_location::nowhere;
}

item_location container_from_parent( const item_location &loc )
{
    return loc;
}

void match( item_location loc, const final_result &result );

template<typename Parent, typename Container>
void match( Parent &&parent, Container &&contents,
            std::vector<final_result> content_results )
{
    CHECK( content_results.size() == contents.size() );
    for( auto &content_maybe_pointer : contents ) {
        item *content = item_pointer( content_maybe_pointer );
        if( content ) {
            INFO( "looking for match in expected result: id = " + content->typeId().str() );
            item_location content_loc( parent, content );
            bool found = false;
            for( auto it = content_results.begin(); it != content_results.end(); ++it ) {
                const final_result &content_result = *it;
                // Assuming all items with the same id are exactly the same for now.
                // Maybe use notes as unique ids instead?
                if( content->typeId() == content_result.id ) {
                    match( content_loc, content_result );
                    item_location container = container_from_parent( parent );
                    if( container ) {
                        item_pocket *pocket = content_loc.parent_pocket();
                        REQUIRE( pocket );
                        CHECK( content_result.parent_pocket_sealed == pocket->sealed() );
                    }
                    found = true;
                    // erase to only match once
                    content_results.erase( it );
                    break;
                }
            }
            INFO( "unexpected result: " << content->typeId().str() );
            // every item in `contents` should have a match
            CHECK( found );
        }
    }
    std::string unfound = "expected result not found: ";
    for( const final_result &content_result : content_results ) {
        unfound += content_result.id.str() + ", ";
    }
    INFO( unfound );
    // every element of `content_results` should have a match
    CHECK( content_results.empty() );
}

void match( item_location loc, const final_result &result )
{
    INFO( "match: id = " << result.id.str() );
    REQUIRE( loc->typeId() == result.id );
    CHECK( result.sealed == loc->any_pockets_sealed() );
    match( loc, loc->all_items_top( item_pocket::pocket_type::CONTAINER ), result.contents );
}

void test_scenario::run()
{
    clear_avatar();
    clear_map();

    const container_location cur_container_loc = value.get<container_location>();
    const scenario cur_scenario = value.get<scenario>();
    const player_action cur_player_action = value.get<player_action>();

    initialization init;
    std::string init_str;
    switch( cur_scenario ) {
        case scenario::contained_liquid: {
            init_str = "scenario::contained_liquid";
            init = {
                initialization {
                    itype_test_watertight_open_sealed_container_250ml,
                    false,
                    true,
                    {
                        initialization {
                            itype_test_liquid_1ml,
                            true,
                            false,
                            {}
                        }
                    }
                }
            };
            break;
        }
        case scenario::nested_contained_liquid: {
            init_str = "scenario::nested_contained_liquid";
            init = {
                initialization {
                    itype_test_watertight_open_sealed_container_1L,
                    false,
                    true,
                    {
                        initialization {
                            itype_test_watertight_open_sealed_multipocket_container_2x250ml,
                            false,
                            true,
                            {
                                initialization {
                                    itype_test_liquid_1ml,
                                    true,
                                    false,
                                    {}
                                }
                            }
                        }
                    }
                }
            };
            break;
        }
        case scenario::recursive_multi_pocket: {
            init_str = "scenario::recursive_multi_pocket";
            init = {
                initialization {
                    itype_test_watertight_open_sealed_multipocket_container_2x1L,
                    false,
                    true,
                    {
                        initialization {
                            itype_test_watertight_open_sealed_multipocket_container_2x250ml,
                            false,
                            true,
                            {
                                initialization {
                                    itype_test_liquid_1ml,
                                    true,
                                    false,
                                    {}
                                }
                            }
                        },
                        initialization {
                            itype_test_solid_1ml,
                            true,
                            false,
                            {}
                        }
                    }
                }
            };
            break;
        }
        default:
            FAIL( "unknown scenario" );
            return;
    }
    // INFO() is scoped, so the message won't be shown if we put in in the switch block.
    INFO( init_str );
    item it = initialize( init );

    map &here = get_map();
    avatar &guy = get_avatar();
    item_location it_loc;
    std::string container_loc_str;
    switch( cur_container_loc ) {
        case container_location::inventory:
            container_loc_str = "container_location::inventory";
            break;
        case container_location::worn:
            container_loc_str = "container_location::worn";
            break;
        case container_location::wielded:
            container_loc_str = "container_location::wielded";
            break;
        case container_location::vehicle:
            container_loc_str = "container_location::vehicle";
            break;
        case container_location::ground:
            container_loc_str = "container_location::ground";
            break;
        default:
            FAIL( "unknown container_location" );
            return;
    }
    // INFO() is scoped, so the message won't be shown if we put in in the switch block.
    INFO( container_loc_str );
    switch( cur_container_loc ) {
        case container_location::inventory: {
            std::optional<std::list<item>::iterator> worn = guy.wear_item( item(
                        itype_test_restricted_container_holder ), false );
            REQUIRE( worn.has_value() );
            ret_val<void> ret = ( **worn ).put_in( it, item_pocket::pocket_type::CONTAINER );
            INFO( ret.str() );
            REQUIRE( ret.success() );
            item_location worn_loc = item_location( guy, & **worn );
            it_loc = item_location( worn_loc, &worn_loc->only_item() );
            break;
        }
        case container_location::worn: {
            std::optional<std::list<item>::iterator> worn = guy.wear_item( it, false );
            REQUIRE( worn.has_value() );
            it_loc = item_location( guy, & **worn );
            break;
        }
        case container_location::wielded: {
            REQUIRE( guy.wield( it ) );
            it_loc = guy.get_wielded_item();
            break;
        }
        case container_location::vehicle: {
            vehicle *veh = here.add_vehicle( vehicle_prototype_test_cargo_space, guy.pos(),
                                             -90_degrees, 0, 0 );
            REQUIRE( veh );
            here.board_vehicle( guy.pos(), &guy );
            std::optional<vpart_reference> vp =
                here.veh_at( guy.pos() ).part_with_feature( vpart_bitflags::VPFLAG_CARGO, true );
            REQUIRE( vp.has_value() );
            std::optional<vehicle_stack::iterator> added = veh->add_item( vp->part(), it );
            REQUIRE( added.has_value() );
            it_loc = item_location( vehicle_cursor( vp->vehicle(), vp->part_index() ), & **added );
            break;
        }
        case container_location::ground: {
            item &added = here.add_item( guy.pos(), it );
            it_loc = item_location( map_cursor( guy.pos() ), &added );
            break;
        }
        default: {
            FAIL( "unknown container_location" );
            return;
        }
    }
    if( !guy.get_wielded_item() ) {
        // so the guy does not wield spilled solid items
        item rag( itype_test_rag );
        REQUIRE( guy.wield( rag ) );
    }

    std::string player_action_str;
    restore_on_out_of_scope<test_mode_spilling_action_t> restore_test_mode_spilling(
        test_mode_spilling_action );
    switch( cur_player_action ) {
        case player_action::spill_all: {
            player_action_str = "player_action::spill_all";
            test_mode_spilling_action = test_mode_spilling_action_t::spill_all;
            break;
        }
        case player_action::cancel_spill: {
            player_action_str = "player_action::cancel_spill";
            test_mode_spilling_action = test_mode_spilling_action_t::cancel_spill;
            break;
        }
        default: {
            FAIL( "unknown player_action" );
            return;
        }
    }
    // INFO() is scoped, so the message won't be shown if we put in in the switch block.
    INFO( player_action_str );

    contents_change_handler handler;
    // TODO replace with actual activities
    unseal_items_containing( handler, it_loc,
                             std::set<itype_id> { itype_test_liquid_1ml, itype_test_solid_1ml } );
    handler.handle_by( guy );

    // check final state
    // whether the outermost container will spill. inner containers will always spill.
    bool will_spill_outer = false;
    switch( cur_container_loc ) {
        case container_location::inventory:
        case container_location::worn:
        case container_location::vehicle:
            will_spill_outer = true;
            break;
        case container_location::wielded:
        case container_location::ground:
            will_spill_outer = false;
            break;
        default: {
            FAIL( "unknown container_location" );
            return;
        }
    }

    bool do_spill = false;
    switch( cur_player_action ) {
        case player_action::spill_all:
            do_spill = true;
            break;
        case player_action::cancel_spill:
            do_spill = false;
            break;
        default: {
            FAIL( "unknown player_action" );
            return;
        }
    }

    std::optional<final_result> original_location;
    std::vector<final_result> ground;
    std::vector<final_result> vehicle_results;
    std::vector<final_result> worn_results;
    std::optional<final_result> wielded_results;
    switch( cur_scenario ) {
        case scenario::contained_liquid:
            if( !will_spill_outer ) {
                original_location = final_result {
                    itype_test_watertight_open_sealed_container_250ml,
                    false,
                    false,
                    {
                        final_result {
                            itype_test_liquid_1ml,
                            false,
                            false,
                            {}
                        }
                    }
                };
            } else if( do_spill ) {
                original_location = final_result {
                    itype_test_watertight_open_sealed_container_250ml,
                    false,
                    false,
                    {}
                };
                ground = {
                    final_result {
                        itype_test_liquid_1ml,
                        false,
                        false,
                        {}
                    }
                };
            } else {
                original_location = std::nullopt;
                ground = {
                    final_result {
                        itype_test_watertight_open_sealed_container_250ml,
                        false,
                        false,
                        {
                            final_result {
                                itype_test_liquid_1ml,
                                false,
                                false,
                                {}
                            }
                        }
                    }
                };
            }
            break;
        case scenario::nested_contained_liquid:
            if( !will_spill_outer && do_spill ) {
                original_location = final_result {
                    itype_test_watertight_open_sealed_container_1L,
                    false,
                    false,
                    {
                        final_result {
                            itype_test_watertight_open_sealed_multipocket_container_2x250ml,
                            false,
                            false,
                            {}
                        }
                    }
                };
                ground = {
                    final_result {
                        itype_test_liquid_1ml,
                        false,
                        false,
                        {}
                    }
                };
            } else if( !do_spill ) {
                original_location = final_result {
                    itype_test_watertight_open_sealed_container_1L,
                    false,
                    false,
                    {}
                };
                ground = {
                    final_result {
                        itype_test_watertight_open_sealed_multipocket_container_2x250ml,
                        false,
                        false,
                        {
                            final_result {
                                itype_test_liquid_1ml,
                                false,
                                false,
                                {}
                            },
                            final_result {
                                itype_test_liquid_1ml,
                                false,
                                false,
                                {}
                            }
                        }
                    }
                };
            } else {
                original_location = final_result {
                    itype_test_watertight_open_sealed_container_1L,
                    false,
                    false,
                    {}
                };
                ground = {
                    final_result {
                        itype_test_liquid_1ml,
                        false,
                        false,
                        {}
                    },
                    final_result {
                        itype_test_watertight_open_sealed_multipocket_container_2x250ml,
                        false,
                        false,
                        {}
                    }
                };
            }
            break;
        case scenario::recursive_multi_pocket:
            if( !will_spill_outer && do_spill ) {
                original_location = final_result {
                    itype_test_watertight_open_sealed_multipocket_container_2x1L,
                    false,
                    false,
                    {
                        final_result {
                            itype_test_watertight_open_sealed_multipocket_container_2x250ml,
                            false,
                            false,
                            {}
                        },
                        final_result {
                            itype_test_solid_1ml,
                            false,
                            false,
                            {}
                        },
                        final_result {
                            itype_test_solid_1ml,
                            false,
                            false,
                            {}
                        }
                    }
                };
                ground = {
                    final_result {
                        itype_test_liquid_1ml,
                        false,
                        false,
                        {}
                    }
                };
            } else if( !will_spill_outer && !do_spill ) {
                original_location = final_result {
                    itype_test_watertight_open_sealed_multipocket_container_2x1L,
                    false,
                    false,
                    {
                        final_result {
                            itype_test_solid_1ml,
                            false,
                            false,
                            {}
                        },
                        final_result {
                            itype_test_solid_1ml,
                            false,
                            false,
                            {}
                        }
                    }
                };
                ground = {
                    final_result {
                        itype_test_watertight_open_sealed_multipocket_container_2x250ml,
                        false,
                        false,
                        {
                            final_result {
                                itype_test_liquid_1ml,
                                false,
                                false,
                                {}
                            },
                            final_result {
                                itype_test_liquid_1ml,
                                false,
                                false,
                                {}
                            }
                        }
                    },
                };
            } else if( do_spill ) {
                original_location = final_result {
                    itype_test_watertight_open_sealed_multipocket_container_2x1L,
                    false,
                    false,
                    {}
                };
                ground = {
                    final_result {
                        itype_test_liquid_1ml,
                        false,
                        false,
                        {}
                    },
                    final_result {
                        itype_test_watertight_open_sealed_multipocket_container_2x250ml,
                        false,
                        false,
                        {}
                    },
                    final_result {
                        itype_test_solid_1ml,
                        false,
                        false,
                        {}
                    }
                };
            } else {
                original_location = final_result {
                    itype_test_watertight_open_sealed_multipocket_container_2x1L,
                    false,
                    false,
                    {}
                };
                ground = {
                    final_result {
                        itype_test_watertight_open_sealed_multipocket_container_2x250ml,
                        false,
                        false,
                        {
                            final_result {
                                itype_test_liquid_1ml,
                                false,
                                false,
                                {}
                            },
                            final_result {
                                itype_test_liquid_1ml,
                                false,
                                false,
                                {}
                            }
                        }
                    },
                    final_result {
                        itype_test_solid_1ml,
                        false,
                        false,
                        {}
                    }
                };
            }
            break;
        default: {
            FAIL( "unknown scenario" );
            return;
        }
    }
    switch( cur_container_loc ) {
        case container_location::ground:
            if( original_location ) {
                ground.emplace_back( *original_location );
            }
            break;
        case container_location::inventory:
            if( original_location ) {
                worn_results.emplace_back( final_result {
                    itype_test_restricted_container_holder,
                    false,
                    false,
                    { *original_location }
                } );
            } else {
                worn_results.emplace_back( final_result {
                    itype_test_restricted_container_holder,
                    false,
                    false,
                    {}
                } );
            }
            break;
        case container_location::worn:
            if( original_location ) {
                worn_results.emplace_back( *original_location );
            }
            break;
        case container_location::vehicle:
            if( original_location ) {
                vehicle_results.emplace_back( *original_location );
            }
            break;
        case container_location::wielded:
            if( original_location ) {
                REQUIRE( !wielded_results.has_value() );
                wielded_results = original_location;
            }
            break;
        default:
            FAIL( "unknown container_location" );
    }
    if( cur_container_loc != container_location::wielded ) {
        REQUIRE( !wielded_results.has_value() );
        wielded_results = final_result {
            itype_test_rag,
            false,
            false,
            {}
        };
    }

    INFO( "checking original item" );
    if( original_location ) {
        REQUIRE( !!it_loc );
        match( it_loc, *original_location );
    } else {
        REQUIRE( !it_loc );
    }
    INFO( "checking ground items" );
    match( map_cursor( guy.pos() ), here.i_at( guy.pos() ), ground );
    INFO( "checking vehicle items" );
    std::optional<vpart_reference> vp = here.veh_at( guy.pos() )
                                        .part_with_feature( vpart_bitflags::VPFLAG_CARGO, true );
    if( cur_container_loc == container_location::vehicle ) {
        REQUIRE( vp.has_value() );
        match( vehicle_cursor( vp->vehicle(), vp->part_index() ),
               vp->vehicle().get_items( vp->part() ), vehicle_results );
    } else {
        REQUIRE( !vp.has_value() );
    }
    INFO( "checking worn items" );
    match( guy, guy.worn.top_items_loc( guy ), worn_results );
    INFO( "checking wielded item" );
    if( wielded_results ) {
        match( guy.get_wielded_item(), *wielded_results );
    } else {
        REQUIRE( !guy.is_armed() );
    }
}
} // namespace

TEST_CASE( "unseal_and_spill" )
{
    test_scenario current = test_scenario::begin();
    const test_scenario end = test_scenario::end();
    while( current != end ) {
        current.run();
        ++current;
    }
}
