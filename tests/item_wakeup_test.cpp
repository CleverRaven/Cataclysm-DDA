#include <algorithm>
#include <cstdint>
#include <functional>
#include <list>
#include <optional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character_id.h"
#include "coordinates.h"
#include "debug.h"
#include "flexbuffer_json.h"
#include "item.h"
#include "item_location.h"
#include "item_uid.h"
#include "item_wakeup.h"
#include "json.h"
#include "json_loader.h"
#include "map.h"
#include "map_helpers.h"
#include "map_selector.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"

static const itype_id itype_backpack( "backpack" );
static const itype_id itype_test_wakeup_dummy( "test_wakeup_dummy" );
static const itype_id itype_test_wakeup_no_handler( "test_wakeup_no_handler" );
static const vproto_id vehicle_prototype_bicycle( "bicycle" );

namespace
{

struct fire_record {
    int64_t uid;
    item_wakeup_kind kind;
    time_point when;
};

std::vector<fire_record> &fire_log()
{
    static std::vector<fire_record> log;
    return log;
}

void dummy_handler( item &it, item_wakeup_kind kind, time_point now,
                    const item_location &/*loc*/ )
{
    fire_log().push_back( fire_record{ it.uid().get_value(), kind, now } );
}

void install_test_handler()
{
    clear_test_wakeup_handlers();
    fire_log().clear();
    register_test_wakeup_handler( itype_test_wakeup_dummy, &dummy_handler );
}

item_locator_hint hint_for_map( const tripoint_abs_ms &p )
{
    item_locator_hint h;
    h.where = item_locator_hint::place::map;
    h.location = p;
    return h;
}

item_locator_hint hint_unknown()
{
    return item_locator_hint{};
}

}  // namespace


TEST_CASE( "item_wakeup_lifecycle_basic", "[item_wakeup][lifecycle]" )
{
    item_wakeup_manager mgr;
    const time_point t = calendar::turn_zero + 5_minutes;
    mgr.schedule_or_update( 1, t, item_wakeup_kind::ready_check, hint_unknown() );

    SECTION( "scheduled item appears in dump" ) {
        REQUIRE( mgr.dump().size() == 1 );
        CHECK( mgr.dump().front().uid == 1 );
        CHECK( mgr.dump().front().kind == item_wakeup_kind::ready_check );
        CHECK( mgr.dump().front().when == t );
    }

    SECTION( "is_scheduled and get reflect the entry" ) {
        CHECK( mgr.is_scheduled( 1, item_wakeup_kind::ready_check ) );
        CHECK_FALSE( mgr.is_scheduled( 1, item_wakeup_kind::alarm ) );
        CHECK( mgr.get( 1, item_wakeup_kind::ready_check ).value() == t );
        CHECK_FALSE( mgr.get( 2, item_wakeup_kind::ready_check ).has_value() );
    }

    SECTION( "cancel removes only matching kind" ) {
        mgr.schedule_or_update( 1, t + 1_minutes, item_wakeup_kind::alarm, hint_unknown() );
        mgr.cancel( 1, item_wakeup_kind::ready_check );
        CHECK_FALSE( mgr.is_scheduled( 1, item_wakeup_kind::ready_check ) );
        CHECK( mgr.is_scheduled( 1, item_wakeup_kind::alarm ) );
    }

    SECTION( "cancel_all removes every kind for uid" ) {
        mgr.schedule_or_update( 1, t + 1_minutes, item_wakeup_kind::alarm, hint_unknown() );
        mgr.schedule_or_update( 2, t, item_wakeup_kind::ready_check, hint_unknown() );
        mgr.cancel_all( 1 );
        CHECK_FALSE( mgr.is_scheduled( 1, item_wakeup_kind::ready_check ) );
        CHECK_FALSE( mgr.is_scheduled( 1, item_wakeup_kind::alarm ) );
        CHECK( mgr.is_scheduled( 2, item_wakeup_kind::ready_check ) );
    }
}

TEST_CASE( "item_wakeup_schedule_or_update_overwrites", "[item_wakeup][lifecycle]" )
{
    item_wakeup_manager mgr;
    const time_point t1 = calendar::turn_zero + 5_minutes;
    const time_point t2 = calendar::turn_zero + 10_minutes;
    mgr.schedule_or_update( 1, t1, item_wakeup_kind::ready_check, hint_unknown() );
    mgr.schedule_or_update( 1, t2, item_wakeup_kind::ready_check, hint_unknown() );

    REQUIRE( mgr.dump().size() == 1 );
    CHECK( mgr.dump().front().when == t2 );
}

TEST_CASE( "item_wakeup_clear_resets_state", "[item_wakeup][lifecycle]" )
{
    item_wakeup_manager mgr;
    mgr.schedule_or_update( 1, calendar::turn_zero + 5_minutes,
                            item_wakeup_kind::ready_check, hint_unknown() );
    mgr.clear();
    CHECK( mgr.dump().empty() );
    CHECK( mgr.get_stats().total_pending == 0 );
}


TEST_CASE( "item_wakeup_fires_at_or_after_when", "[item_wakeup][firing]" )
{
    clear_map();
    install_test_handler();

    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    item placed = item( itype_test_wakeup_dummy, calendar::turn );
    item &on_map = here.add_item( origin, placed );
    const int64_t uid = on_map.uid().get_value();

    item_wakeup_manager mgr;
    const time_point fire_at = calendar::turn + 5_minutes;
    mgr.schedule_or_update( uid, fire_at, item_wakeup_kind::ready_check,
                            hint_for_map( here.get_abs( origin ) ) );

    SECTION( "before fire time, nothing fires" ) {
        mgr.process( fire_at - 1_minutes );
        CHECK( fire_log().empty() );
        CHECK( mgr.is_scheduled( uid, item_wakeup_kind::ready_check ) );
    }

    SECTION( "at fire time, handler fires once and entry is consumed" ) {
        mgr.process( fire_at );
        REQUIRE( fire_log().size() == 1 );
        CHECK( fire_log().front().uid == uid );
        CHECK( fire_log().front().kind == item_wakeup_kind::ready_check );
        CHECK_FALSE( mgr.is_scheduled( uid, item_wakeup_kind::ready_check ) );
    }

    SECTION( "process well after fire time still fires once" ) {
        mgr.process( fire_at + 1_hours );
        CHECK( fire_log().size() == 1 );
    }
}

TEST_CASE( "item_wakeup_fires_in_time_order", "[item_wakeup][firing]" )
{
    clear_map();
    install_test_handler();

    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    item &a = here.add_item( origin, item( itype_test_wakeup_dummy, calendar::turn ) );
    item &b = here.add_item( origin, item( itype_test_wakeup_dummy, calendar::turn ) );
    const int64_t uid_a = a.uid().get_value();
    const int64_t uid_b = b.uid().get_value();

    item_wakeup_manager mgr;
    const time_point t1 = calendar::turn + 1_minutes;
    const time_point t2 = calendar::turn + 2_minutes;
    const item_locator_hint h = hint_for_map( here.get_abs( origin ) );
    mgr.schedule_or_update( uid_b, t2, item_wakeup_kind::ready_check, h );
    mgr.schedule_or_update( uid_a, t1, item_wakeup_kind::ready_check, h );

    mgr.process( t2 );
    REQUIRE( fire_log().size() == 2 );
    // Handlers receive `now`, not the original wakeup time, so uid order is
    // the contract: earlier-scheduled wakeups dispatch first.
    CHECK( fire_log()[0].uid == uid_a );
    CHECK( fire_log()[1].uid == uid_b );
}

TEST_CASE( "item_wakeup_same_pass_reschedule_does_not_double_fire",
           "[item_wakeup][firing]" )
{
    clear_map();
    clear_test_wakeup_handlers();
    fire_log().clear();

    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    item &on_map = here.add_item( origin, item( itype_test_wakeup_dummy, calendar::turn ) );
    const int64_t uid = on_map.uid().get_value();

    item_wakeup_manager *mgr_ptr = nullptr;
    auto rescheduling_handler = []( item & it, item_wakeup_kind kind, time_point now,
    const item_location & /*loc*/ ) {
        fire_log().push_back( fire_record{ it.uid().get_value(), kind, now } );
        // Item handler reschedules itself for "now"; this MUST NOT fire again
        // in the same process pass.
        get_item_wakeups().schedule_or_update( it.uid().get_value(), now,
                                               item_wakeup_kind::ready_check, hint_unknown() );
    };
    register_test_wakeup_handler( itype_test_wakeup_dummy, rescheduling_handler );

    item_wakeup_manager &mgr = get_item_wakeups();
    mgr.clear();
    mgr_ptr = &mgr;
    const time_point t = calendar::turn + 1_minutes;
    mgr.schedule_or_update( uid, t, item_wakeup_kind::ready_check,
                            hint_for_map( here.get_abs( origin ) ) );

    mgr.process( t );
    CHECK( fire_log().size() == 1 );
    // Re-scheduled entry remains pending until the next process pass.
    CHECK( mgr_ptr->is_scheduled( uid, item_wakeup_kind::ready_check ) );
    mgr.clear();
    clear_test_wakeup_handlers();
}


TEST_CASE( "item_wakeup_handler_called_once_per_fire", "[item_wakeup][idempotency]" )
{
    clear_map();
    install_test_handler();

    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    item &on_map = here.add_item( origin, item( itype_test_wakeup_dummy, calendar::turn ) );
    const int64_t uid = on_map.uid().get_value();

    item_wakeup_manager mgr;
    const time_point t = calendar::turn + 1_minutes;
    mgr.schedule_or_update( uid, t, item_wakeup_kind::ready_check,
                            hint_for_map( here.get_abs( origin ) ) );
    mgr.process( t );
    mgr.process( t + 1_hours );

    CHECK( fire_log().size() == 1 );
}


TEST_CASE( "item_wakeup_default_enumerate_is_empty", "[item_wakeup][rebuild]" )
{
    item it( itype_test_wakeup_dummy, calendar::turn );
    CHECK( it.enumerate_scheduled_wakeups( item_location() ).empty() );
}

TEST_CASE( "item_wakeup_rebuild_schedules_from_item", "[item_wakeup][rebuild]" )
{
    // Default-enumerate is empty, so rebuild_for_item must cancel any
    // pre-existing entry for the item's uid.
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    item &on_map = here.add_item( origin, item( itype_test_wakeup_dummy, calendar::turn ) );
    const int64_t uid = on_map.uid().get_value();

    item_wakeup_manager mgr;
    mgr.schedule_or_update( uid, calendar::turn + 5_minutes,
                            item_wakeup_kind::ready_check,
                            hint_for_map( here.get_abs( origin ) ) );
    REQUIRE( mgr.is_scheduled( uid, item_wakeup_kind::ready_check ) );

    mgr.rebuild_for_item( item_location( map_cursor( here.get_abs( origin ) ), &on_map ) );
    CHECK_FALSE( mgr.is_scheduled( uid, item_wakeup_kind::ready_check ) );
}


TEST_CASE( "item_wakeup_resolve_on_map", "[item_wakeup][resolve]" )
{
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    item &on_map = here.add_item( origin, item( itype_test_wakeup_dummy, calendar::turn ) );
    const int64_t uid = on_map.uid().get_value();

    item_location loc = find_item_by_uid( uid, hint_for_map( here.get_abs( origin ) ) );
    REQUIRE( loc );
    CHECK( loc.get_item()->uid().get_value() == uid );
}

TEST_CASE( "item_wakeup_resolve_on_character", "[item_wakeup][resolve]" )
{
    clear_avatar();
    avatar &u = get_avatar();
    item carried( itype_test_wakeup_dummy, calendar::turn );
    item_location placed = u.i_add( carried );
    REQUIRE( placed );
    const int64_t uid = placed->uid().get_value();

    item_locator_hint hint;
    hint.where = item_locator_hint::place::character;
    hint.location = u.getID();

    item_location loc = find_item_by_uid( uid, hint );
    REQUIRE( loc );
    CHECK( loc.get_item()->uid().get_value() == uid );
}

TEST_CASE( "item_wakeup_resolve_destroyed_item_returns_invalid", "[item_wakeup][resolve]" )
{
    clear_map();
    item_location loc = find_item_by_uid( 999999999, hint_unknown() );
    CHECK_FALSE( loc );
}

TEST_CASE( "item_wakeup_resolve_invalid_uid_returns_invalid", "[item_wakeup][resolve]" )
{
    item_location loc = find_item_by_uid( 0, hint_unknown() );
    CHECK_FALSE( loc );
}

TEST_CASE( "item_wakeup_resolve_falls_back_to_loaded_map", "[item_wakeup][resolve]" )
{
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms elsewhere( 65, 65, 0 );
    u.setpos( here, origin );
    item &on_map = here.add_item( elsewhere,
                                  item( itype_test_wakeup_dummy, calendar::turn ) );
    const int64_t uid = on_map.uid().get_value();

    // Hint is wrong (points to origin, item is at elsewhere).  Map fallback
    // should still resolve via reality-bubble scan.
    item_location loc = find_item_by_uid( uid, hint_for_map( here.get_abs( origin ) ) );
    REQUIRE( loc );
    CHECK( loc.get_item()->uid().get_value() == uid );
}

TEST_CASE( "item_wakeup_wrong_hint_falls_back_to_avatar", "[item_wakeup][resolve]" )
{
    clear_avatar();
    avatar &u = get_avatar();
    item carried( itype_test_wakeup_dummy, calendar::turn );
    item_location placed = u.i_add( carried );
    REQUIRE( placed );
    const int64_t uid = placed->uid().get_value();

    // Hint says map, but item is in inventory.  Fallback should find it.
    item_location loc = find_item_by_uid( uid,
                                          hint_for_map( tripoint_abs_ms{ 1, 1, 0 } ) );
    REQUIRE( loc );
    CHECK( loc.get_item()->uid().get_value() == uid );
}


TEST_CASE( "item_wakeup_save_load_roundtrip", "[item_wakeup][persistence]" )
{
    item_wakeup_manager src;
    const time_point t = calendar::turn_zero + 10_minutes;
    src.schedule_or_update( 100, t, item_wakeup_kind::ready_check,
                            hint_for_map( tripoint_abs_ms{ 1, 2, 0 } ) );
    src.schedule_or_update( 100, t + 1_minutes, item_wakeup_kind::alarm, hint_unknown() );
    src.schedule_or_update( 200, t + 2_minutes, item_wakeup_kind::fail_check, hint_unknown() );

    std::ostringstream os;
    JsonOut jsout( os );
    src.serialize( jsout );

    item_wakeup_manager dst;
    JsonValue jv = json_loader::from_string( os.str() );
    JsonObject jo = jv.get_object();
    dst.deserialize( jo );

    REQUIRE( dst.dump().size() == 3 );
    CHECK( dst.is_scheduled( 100, item_wakeup_kind::ready_check ) );
    CHECK( dst.is_scheduled( 100, item_wakeup_kind::alarm ) );
    CHECK( dst.is_scheduled( 200, item_wakeup_kind::fail_check ) );
}

TEST_CASE( "item_wakeup_save_load_character_hint", "[item_wakeup][persistence]" )
{
    item_wakeup_manager src;
    item_locator_hint hint;
    hint.where = item_locator_hint::place::character;
    hint.location = character_id( 42 );
    src.schedule_or_update( 7, calendar::turn_zero + 1_minutes,
                            item_wakeup_kind::alarm, hint );

    std::ostringstream os;
    JsonOut jsout( os );
    src.serialize( jsout );

    item_wakeup_manager dst;
    JsonValue jv = json_loader::from_string( os.str() );
    JsonObject jo = jv.get_object();
    dst.deserialize( jo );

    REQUIRE( dst.is_scheduled( 7, item_wakeup_kind::alarm ) );
}

TEST_CASE( "item_wakeup_save_load_vehicle_hint", "[item_wakeup][persistence]" )
{
    item_wakeup_manager src;
    vehicle_hint vh;
    vh.cargo_square = tripoint_abs_ms{ 10, 20, 0 };
    vh.part_index = 3;
    vh.mount_offset = point_rel_ms{ 1, 0 };
    item_locator_hint hint;
    hint.where = item_locator_hint::place::vehicle;
    hint.location = vh;
    src.schedule_or_update( 99, calendar::turn_zero + 5_minutes,
                            item_wakeup_kind::ready_check, hint );

    std::ostringstream os;
    JsonOut jsout( os );
    src.serialize( jsout );

    item_wakeup_manager dst;
    JsonValue jv = json_loader::from_string( os.str() );
    JsonObject jo = jv.get_object();
    dst.deserialize( jo );

    REQUIRE( dst.is_scheduled( 99, item_wakeup_kind::ready_check ) );
}

TEST_CASE( "item_wakeup_load_absent_version_treats_as_v1",
           "[item_wakeup][persistence]" )
{
    const std::string raw = R"(
        {
            "wakeups": [
                { "uid": 5, "kind": "ready_check", "when": 100 }
            ]
        }
    )";

    item_wakeup_manager mgr;
    const std::string dmsg = capture_debugmsg_during( [&]() {
        JsonValue jv = json_loader::from_string( raw );
        JsonObject jo = jv.get_object();
        mgr.deserialize( jo );
    } );
    CHECK_FALSE( dmsg.empty() );
    CHECK( mgr.is_scheduled( 5, item_wakeup_kind::ready_check ) );
}

TEST_CASE( "item_wakeup_load_dedupe_increments_duplicate_event_counter",
           "[item_wakeup][persistence]" )
{
    const std::string raw = R"(
        {
            "version": 1,
            "wakeups": [
                { "uid": 5, "kind": "ready_check", "when": 100 },
                { "uid": 5, "kind": "ready_check", "when": 200 }
            ]
        }
    )";

    item_wakeup_manager mgr;
    JsonValue jv = json_loader::from_string( raw );
    JsonObject jo = jv.get_object();
    mgr.deserialize( jo );

    CHECK( mgr.dump().size() == 1 );
    CHECK( mgr.get_stats().duplicate_event == 1 );
}

TEST_CASE( "item_wakeup_load_drops_bad_entries", "[item_wakeup][persistence]" )
{
    const std::string raw = R"(
        {
            "version": 1,
            "wakeups": [
                { "uid": 0, "kind": "alarm", "when": 100 },
                { "uid": 5, "kind": "ready_check", "when": 100 },
                { "uid": 7, "kind": "garbage_kind", "when": 100 }
            ]
        }
    )";

    item_wakeup_manager mgr;
    JsonValue jv = json_loader::from_string( raw );
    JsonObject jo = jv.get_object();
    mgr.deserialize( jo );

    CHECK( mgr.dump().size() == 1 );
    CHECK( mgr.is_scheduled( 5, item_wakeup_kind::ready_check ) );
    CHECK( mgr.get_stats().dropped_on_load == 2 );
}

TEST_CASE( "item_wakeup_load_unknown_version_drops_queue",
           "[item_wakeup][persistence]" )
{
    const std::string raw = R"(
        {
            "version": 999,
            "wakeups": [
                { "uid": 5, "kind": "ready_check", "when": 100 }
            ]
        }
    )";

    item_wakeup_manager mgr;
    const std::string dmsg = capture_debugmsg_during( [&]() {
        JsonValue jv = json_loader::from_string( raw );
        JsonObject jo = jv.get_object();
        mgr.deserialize( jo );
    } );
    CHECK_FALSE( dmsg.empty() );
    CHECK( mgr.dump().empty() );
}

TEST_CASE( "item_wakeup_load_dedupes_uid_kind_keeping_latest",
           "[item_wakeup][persistence]" )
{
    const std::string raw = R"(
        {
            "version": 1,
            "wakeups": [
                { "uid": 5, "kind": "ready_check", "when": 100 },
                { "uid": 5, "kind": "ready_check", "when": 200 }
            ]
        }
    )";

    item_wakeup_manager mgr;
    JsonValue jv = json_loader::from_string( raw );
    JsonObject jo = jv.get_object();
    mgr.deserialize( jo );

    REQUIRE( mgr.dump().size() == 1 );
    const time_point persisted = mgr.get( 5, item_wakeup_kind::ready_check ).value();
    CHECK( to_turn<int>( persisted ) == 200 );
}


TEST_CASE( "item_wakeup_dump_is_sorted_deterministically", "[item_wakeup][diag]" )
{
    item_wakeup_manager mgr;
    const time_point t = calendar::turn_zero + 5_minutes;
    mgr.schedule_or_update( 3, t + 1_minutes, item_wakeup_kind::ready_check, hint_unknown() );
    mgr.schedule_or_update( 1, t + 1_minutes, item_wakeup_kind::ready_check, hint_unknown() );
    mgr.schedule_or_update( 2, t, item_wakeup_kind::alarm, hint_unknown() );
    mgr.schedule_or_update( 2, t, item_wakeup_kind::ready_check, hint_unknown() );

    std::vector<scheduled_wakeup_info> d = mgr.dump();
    REQUIRE( d.size() == 4 );
    // Sorted by (when, uid, kind)
    CHECK( d[0].uid == 2 );
    CHECK( d[0].kind == item_wakeup_kind::alarm );
    CHECK( d[1].uid == 2 );
    CHECK( d[1].kind == item_wakeup_kind::ready_check );
    CHECK( d[2].uid == 1 );
    CHECK( d[3].uid == 3 );
}

TEST_CASE( "item_wakeup_peek_matches_dump_front", "[item_wakeup][diag]" )
{
    item_wakeup_manager mgr;
    const time_point t = calendar::turn_zero + 5_minutes;

    SECTION( "empty manager returns nullopt" ) {
        CHECK_FALSE( mgr.peek_next_wakeup().has_value() );
    }

    SECTION( "non-empty manager matches dump front" ) {
        mgr.schedule_or_update( 5, t, item_wakeup_kind::ready_check, hint_unknown() );
        mgr.schedule_or_update( 3, t + 1_minutes, item_wakeup_kind::alarm, hint_unknown() );
        std::optional<scheduled_wakeup_info> peek = mgr.peek_next_wakeup();
        REQUIRE( peek.has_value() );
        CHECK( peek->uid == mgr.dump().front().uid );
        CHECK( peek->kind == mgr.dump().front().kind );
        CHECK( peek->when == mgr.dump().front().when );
    }
}

TEST_CASE( "item_wakeup_total_pending_tracks_entry_count", "[item_wakeup][diag]" )
{
    item_wakeup_manager mgr;
    CHECK( mgr.get_stats().total_pending == 0 );
    mgr.schedule_or_update( 1, calendar::turn_zero + 1_minutes,
                            item_wakeup_kind::alarm, hint_unknown() );
    CHECK( mgr.get_stats().total_pending == 1 );
    mgr.schedule_or_update( 2, calendar::turn_zero + 2_minutes,
                            item_wakeup_kind::alarm, hint_unknown() );
    CHECK( mgr.get_stats().total_pending == 2 );
    mgr.cancel( 1, item_wakeup_kind::alarm );
    CHECK( mgr.get_stats().total_pending == 1 );
}


TEST_CASE( "item_wakeup_no_registered_handler_is_noop",
           "[item_wakeup][firing]" )
{
    clear_map();
    clear_test_wakeup_handlers();
    fire_log().clear();

    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    item &on_map = here.add_item( origin,
                                  item( itype_test_wakeup_no_handler, calendar::turn ) );
    const int64_t uid = on_map.uid().get_value();

    item_wakeup_manager mgr;
    mgr.schedule_or_update( uid, calendar::turn + 1_minutes,
                            item_wakeup_kind::ready_check,
                            hint_for_map( here.get_abs( origin ) ) );
    mgr.process( calendar::turn + 5_minutes );

    CHECK( fire_log().empty() );
    CHECK_FALSE( mgr.is_scheduled( uid, item_wakeup_kind::ready_check ) );
}


namespace
{
std::vector<desired_wakeup> dummy_enumerator( const item &/*it*/,
        const item_location &/*loc*/ )
{
    return {
        desired_wakeup{ item_wakeup_kind::ready_check, calendar::turn_zero + 5_minutes },
        desired_wakeup{ item_wakeup_kind::fail_check, calendar::turn_zero + 10_minutes },
    };
}
}  // namespace

TEST_CASE( "item_wakeup_rebuild_schedules_from_producer", "[item_wakeup][rebuild]" )
{
    clear_map();
    clear_test_enumerate_handlers();
    register_test_enumerate_handler( itype_test_wakeup_dummy, &dummy_enumerator );

    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    item &on_map = here.add_item( origin, item( itype_test_wakeup_dummy, calendar::turn ) );
    const int64_t uid = on_map.uid().get_value();

    item_wakeup_manager mgr;
    mgr.rebuild_for_item( item_location( map_cursor( here.get_abs( origin ) ), &on_map ) );

    CHECK( mgr.is_scheduled( uid, item_wakeup_kind::ready_check ) );
    CHECK( mgr.is_scheduled( uid, item_wakeup_kind::fail_check ) );
    CHECK( mgr.get( uid, item_wakeup_kind::ready_check ).value()
           == calendar::turn_zero + 5_minutes );

    clear_test_enumerate_handlers();
}

TEST_CASE( "item_wakeup_rebuild_updates_existing_when", "[item_wakeup][rebuild]" )
{
    clear_map();
    clear_test_enumerate_handlers();
    register_test_enumerate_handler( itype_test_wakeup_dummy, &dummy_enumerator );

    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    item &on_map = here.add_item( origin, item( itype_test_wakeup_dummy, calendar::turn ) );
    const int64_t uid = on_map.uid().get_value();

    item_wakeup_manager mgr;
    // Pre-existing entry with a different time.
    mgr.schedule_or_update( uid, calendar::turn_zero + 1_minutes,
                            item_wakeup_kind::ready_check,
                            hint_for_map( here.get_abs( origin ) ) );

    mgr.rebuild_for_item( item_location( map_cursor( here.get_abs( origin ) ), &on_map ) );

    CHECK( mgr.get( uid, item_wakeup_kind::ready_check ).value()
           == calendar::turn_zero + 5_minutes );
    clear_test_enumerate_handlers();
}

TEST_CASE( "item_wakeup_resolve_on_vehicle_cargo", "[item_wakeup][resolve][vehicle]" )
{
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    const tripoint_bub_ms veh_pos( 65, 65, 0 );
    vehicle *veh = here.add_vehicle( vehicle_prototype_bicycle, veh_pos, 0_degrees, -1, 2 );
    REQUIRE( veh != nullptr );

    int cargo_part_idx = -1;
    for( const vpart_reference &vpr : veh->get_all_parts() ) {
        if( vpr.has_feature( "CARGO" ) ) {
            cargo_part_idx = vpr.part_index();
            break;
        }
    }
    REQUIRE( cargo_part_idx >= 0 );

    vehicle_part &cargo_part = veh->part( cargo_part_idx );
    item dummy( itype_test_wakeup_dummy, calendar::turn );
    std::optional<vehicle_stack::iterator> added =
        veh->add_item( here, cargo_part, dummy );
    REQUIRE( added.has_value() );
    const int64_t uid = ( *added )->uid().get_value();

    vehicle_hint vh;
    vh.cargo_square = here.get_abs( veh->bub_part_pos( here, cargo_part ) );
    vh.part_index = cargo_part_idx;
    vh.mount_offset = cargo_part.mount;
    item_locator_hint hint;
    hint.where = item_locator_hint::place::vehicle;
    hint.location = vh;

    item_location loc = find_item_by_uid( uid, hint );
    REQUIRE( loc );
    CHECK( loc.get_item()->uid().get_value() == uid );
}

TEST_CASE( "item_wakeup_resolve_vehicle_stale_part_index_uses_mount",
           "[item_wakeup][resolve][vehicle]" )
{
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    const tripoint_bub_ms veh_pos( 65, 65, 0 );
    vehicle *veh = here.add_vehicle( vehicle_prototype_bicycle, veh_pos, 0_degrees, -1, 2 );
    REQUIRE( veh != nullptr );

    int cargo_part_idx = -1;
    for( const vpart_reference &vpr : veh->get_all_parts() ) {
        if( vpr.has_feature( "CARGO" ) ) {
            cargo_part_idx = vpr.part_index();
            break;
        }
    }
    REQUIRE( cargo_part_idx >= 0 );

    vehicle_part &cargo_part = veh->part( cargo_part_idx );
    const point_rel_ms saved_mount = cargo_part.mount;
    std::optional<vehicle_stack::iterator> added =
        veh->add_item( here, cargo_part,
                       item( itype_test_wakeup_dummy, calendar::turn ) );
    REQUIRE( added.has_value() );
    const int64_t uid = ( *added )->uid().get_value();

    vehicle_hint vh;
    vh.cargo_square = here.get_abs( veh->bub_part_pos( here, cargo_part ) );
    // Deliberately wrong part_index but valid mount_offset.
    vh.part_index = veh->part_count() + 100;
    vh.mount_offset = saved_mount;
    item_locator_hint hint;
    hint.where = item_locator_hint::place::vehicle;
    hint.location = vh;

    item_location loc = find_item_by_uid( uid, hint );
    REQUIRE( loc );
    CHECK( loc.get_item()->uid().get_value() == uid );
}

TEST_CASE( "item_wakeup_resolve_vehicle_moved_uses_loaded_scan",
           "[item_wakeup][resolve][vehicle]" )
{
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    const tripoint_bub_ms initial_pos( 65, 65, 0 );
    vehicle *veh = here.add_vehicle( vehicle_prototype_bicycle, initial_pos,
                                     0_degrees, -1, 2 );
    REQUIRE( veh != nullptr );

    int cargo_part_idx = -1;
    for( const vpart_reference &vpr : veh->get_all_parts() ) {
        if( vpr.has_feature( "CARGO" ) ) {
            cargo_part_idx = vpr.part_index();
            break;
        }
    }
    REQUIRE( cargo_part_idx >= 0 );

    vehicle_part &cargo_part = veh->part( cargo_part_idx );
    const tripoint_abs_ms saved_square = here.get_abs(
            veh->bub_part_pos( here, cargo_part ) );
    std::optional<vehicle_stack::iterator> added =
        veh->add_item( here, cargo_part,
                       item( itype_test_wakeup_dummy, calendar::turn ) );
    REQUIRE( added.has_value() );
    const int64_t uid = ( *added )->uid().get_value();

    // Move the vehicle one tile so the saved cargo_square no longer matches.
    here.displace_vehicle( *veh, tripoint_rel_ms{ 1, 0, 0 } );

    vehicle_hint vh;
    vh.cargo_square = saved_square;
    vh.part_index = cargo_part_idx;
    vh.mount_offset = cargo_part.mount;
    item_locator_hint hint;
    hint.where = item_locator_hint::place::vehicle;
    hint.location = vh;

    item_location loc = find_item_by_uid( uid, hint );
    REQUIRE( loc );
    CHECK( loc.get_item()->uid().get_value() == uid );
}


TEST_CASE( "item_wakeup_resolve_off_bubble_returns_invalid",
           "[item_wakeup][resolve]" )
{
    clear_map();
    // tripoint_abs_ms far from the reality bubble origin.  inbounds() must
    // reject it; resolution returns invalid without crashing.
    const tripoint_abs_ms far_away{ 1000000, 1000000, 0 };
    item_location loc = find_item_by_uid( 12345, hint_for_map( far_away ) );
    CHECK_FALSE( loc );
}

TEST_CASE( "item_wakeup_update_displaces_top", "[item_wakeup][heap]" )
{
    item_wakeup_manager mgr;
    const time_point t_early = calendar::turn_zero + 1_minutes;
    const time_point t_late = calendar::turn_zero + 10_minutes;

    mgr.schedule_or_update( 5, t_early, item_wakeup_kind::ready_check, hint_unknown() );
    mgr.schedule_or_update( 9, t_early + 1_minutes, item_wakeup_kind::ready_check,
                            hint_unknown() );
    mgr.schedule_or_update( 5, t_late, item_wakeup_kind::ready_check, hint_unknown() );

    std::optional<scheduled_wakeup_info> top = mgr.peek_next_wakeup();
    REQUIRE( top.has_value() );
    CHECK( top->uid == 9 );
    CHECK( top->when == t_early + 1_minutes );
    CHECK( mgr.get_stats().total_pending == 2 );
}

TEST_CASE( "item_wakeup_cancel_top_lets_next_fire", "[item_wakeup][heap]" )
{
    item_wakeup_manager mgr;
    const time_point t_early = calendar::turn_zero + 1_minutes;
    const time_point t_late = calendar::turn_zero + 5_minutes;

    mgr.schedule_or_update( 1, t_early, item_wakeup_kind::ready_check, hint_unknown() );
    mgr.schedule_or_update( 2, t_late, item_wakeup_kind::ready_check, hint_unknown() );
    mgr.cancel( 1, item_wakeup_kind::ready_check );

    std::optional<scheduled_wakeup_info> top = mgr.peek_next_wakeup();
    REQUIRE( top.has_value() );
    CHECK( top->uid == 2 );
    CHECK( top->when == t_late );
}

TEST_CASE( "item_wakeup_serialize_after_update_drops_tombstones",
           "[item_wakeup][heap][persistence]" )
{
    item_wakeup_manager src;
    src.schedule_or_update( 5, calendar::turn_zero + 1_minutes,
                            item_wakeup_kind::ready_check, hint_unknown() );
    src.schedule_or_update( 5, calendar::turn_zero + 10_minutes,
                            item_wakeup_kind::ready_check, hint_unknown() );
    src.cancel( 5, item_wakeup_kind::ready_check );

    std::ostringstream os;
    JsonOut jsout( os );
    src.serialize( jsout );

    item_wakeup_manager dst;
    JsonValue jv = json_loader::from_string( os.str() );
    JsonObject jo = jv.get_object();
    dst.deserialize( jo );

    CHECK( dst.get_stats().total_pending == 0 );
    CHECK_FALSE( dst.is_scheduled( 5, item_wakeup_kind::ready_check ) );
}

TEST_CASE( "item_wakeup_peek_tiebreak_matches_dump_order", "[item_wakeup][heap]" )
{
    item_wakeup_manager mgr;
    const time_point t = calendar::turn_zero + 5_minutes;
    mgr.schedule_or_update( 5, t, item_wakeup_kind::ready_check, hint_unknown() );
    mgr.schedule_or_update( 5, t, item_wakeup_kind::alarm, hint_unknown() );
    mgr.schedule_or_update( 3, t, item_wakeup_kind::ready_check, hint_unknown() );
    mgr.schedule_or_update( 3, t, item_wakeup_kind::alarm, hint_unknown() );

    const std::vector<scheduled_wakeup_info> sorted = mgr.dump();
    REQUIRE( sorted.size() == 4 );
    const std::optional<scheduled_wakeup_info> top = mgr.peek_next_wakeup();
    REQUIRE( top.has_value() );
    CHECK( top->uid == sorted.front().uid );
    CHECK( top->kind == sorted.front().kind );
    CHECK( top->when == sorted.front().when );
}

TEST_CASE( "item_wakeup_dispatch_order_with_equal_when", "[item_wakeup][heap]" )
{
    clear_map();
    install_test_handler();

    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    item &a = here.add_item( origin, item( itype_test_wakeup_dummy, calendar::turn ) );
    item &b = here.add_item( origin, item( itype_test_wakeup_dummy, calendar::turn ) );
    const int64_t uid_a = a.uid().get_value();
    const int64_t uid_b = b.uid().get_value();
    const int64_t small_uid = std::min( uid_a, uid_b );
    const int64_t large_uid = std::max( uid_a, uid_b );

    item_wakeup_manager mgr;
    const time_point fire_at = calendar::turn + 1_minutes;
    const item_locator_hint h = hint_for_map( here.get_abs( origin ) );
    mgr.schedule_or_update( large_uid, fire_at, item_wakeup_kind::ready_check, h );
    mgr.schedule_or_update( small_uid, fire_at, item_wakeup_kind::ready_check, h );

    mgr.process( fire_at );
    REQUIRE( fire_log().size() == 2 );
    CHECK( fire_log()[0].uid == small_uid );
    CHECK( fire_log()[1].uid == large_uid );
}

TEST_CASE( "item_wakeup_deserialize_restores_heap_invariant",
           "[item_wakeup][heap][persistence]" )
{
    item_wakeup_manager src;
    src.schedule_or_update( 9, calendar::turn_zero + 5_minutes,
                            item_wakeup_kind::ready_check, hint_unknown() );
    src.schedule_or_update( 1, calendar::turn_zero + 1_minutes,
                            item_wakeup_kind::ready_check, hint_unknown() );

    std::ostringstream os;
    JsonOut jsout( os );
    src.serialize( jsout );

    item_wakeup_manager dst;
    JsonValue jv = json_loader::from_string( os.str() );
    JsonObject jo = jv.get_object();
    dst.deserialize( jo );

    std::optional<scheduled_wakeup_info> top = dst.peek_next_wakeup();
    REQUIRE( top.has_value() );
    CHECK( top->uid == 1 );
    CHECK( top->when == calendar::turn_zero + 1_minutes );
}

namespace
{
struct location_record {
    int64_t uid = 0;
    item_location::type where = item_location::type::invalid;
    tripoint_abs_ms abs;
};

std::vector<location_record> &location_log()
{
    static std::vector<location_record> log;
    return log;
}

void location_capturing_handler( item &it, item_wakeup_kind /*kind*/,
                                 time_point /*now*/, const item_location &loc )
{
    location_record r;
    r.uid = it.uid().get_value();
    if( loc ) {
        r.where = loc.where();
        r.abs = loc.pos_abs();
    }
    location_log().push_back( r );
}
}  // namespace

TEST_CASE( "item_wakeup_handler_observes_resolved_location",
           "[item_wakeup][location]" )
{
    clear_map();
    clear_test_wakeup_handlers();
    location_log().clear();
    register_test_wakeup_handler( itype_test_wakeup_dummy, &location_capturing_handler );

    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    item &on_map = here.add_item( origin, item( itype_test_wakeup_dummy, calendar::turn ) );
    const int64_t uid = on_map.uid().get_value();
    const tripoint_abs_ms expected_abs = here.get_abs( origin );

    item_wakeup_manager mgr;
    const time_point t = calendar::turn + 1_minutes;
    mgr.schedule_or_update( uid, t, item_wakeup_kind::ready_check,
                            hint_for_map( expected_abs ) );
    mgr.process( t );

    REQUIRE( location_log().size() == 1 );
    CHECK( location_log()[0].uid == uid );
    CHECK( location_log()[0].where == item_location::type::map );
    CHECK( location_log()[0].abs == expected_abs );

    clear_test_wakeup_handlers();
}

TEST_CASE( "item_wakeup_enumerator_observes_resolved_location",
           "[item_wakeup][location][rebuild]" )
{
    static std::vector<tripoint_abs_ms> seen_locs;
    seen_locs.clear();
    auto loc_enumerator = []( const item & /*it*/, const item_location & loc )
    -> std::vector<desired_wakeup> {
        if( loc )
        {
            seen_locs.push_back( loc.pos_abs() );
        }
        return {
            desired_wakeup{ item_wakeup_kind::ready_check, calendar::turn_zero + 5_minutes },
        };
    };

    clear_map();
    clear_test_enumerate_handlers();
    register_test_enumerate_handler( itype_test_wakeup_dummy, loc_enumerator );

    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    item &on_map = here.add_item( origin, item( itype_test_wakeup_dummy, calendar::turn ) );
    const tripoint_abs_ms expected_abs = here.get_abs( origin );

    item_wakeup_manager mgr;
    mgr.rebuild_for_item( item_location( map_cursor( expected_abs ), &on_map ) );

    REQUIRE( seen_locs.size() == 1 );
    CHECK( seen_locs[0] == expected_abs );

    clear_test_enumerate_handlers();
}

TEST_CASE( "item_wakeup_map_load_reconciles_dropped_entries",
           "[item_wakeup][reconcile]" )
{
    auto reconcile_enumerator = []( const item & /*it*/, const item_location & /*loc*/ )
    -> std::vector<desired_wakeup> {
        return {
            desired_wakeup{ item_wakeup_kind::ready_check, calendar::turn_zero + 30_minutes },
        };
    };

    clear_test_enumerate_handlers();
    register_test_enumerate_handler( itype_test_wakeup_dummy, reconcile_enumerator );

    item_wakeup_manager &mgr = get_item_wakeups();
    mgr.clear();

    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    item &on_map = here.add_item( origin, item( itype_test_wakeup_dummy, calendar::turn ) );
    const int64_t uid = on_map.uid().get_value();

    mgr.clear();
    REQUIRE_FALSE( mgr.is_scheduled( uid, item_wakeup_kind::ready_check ) );

    here.load( here.get_abs_sub(), /*update_vehicles=*/false, /*pump_events=*/false );

    CHECK( mgr.is_scheduled( uid, item_wakeup_kind::ready_check ) );
    if( mgr.is_scheduled( uid, item_wakeup_kind::ready_check ) ) {
        CHECK( mgr.get( uid, item_wakeup_kind::ready_check ).value()
               == calendar::turn_zero + 30_minutes );
    }

    mgr.clear();
    clear_test_enumerate_handlers();
}

TEST_CASE( "item_wakeup_shift_reconciles_dropped_entries",
           "[item_wakeup][reconcile][shift]" )
{
    auto enumerator = []( const item & /*it*/, const item_location & /*loc*/ )
    -> std::vector<desired_wakeup> {
        return {
            desired_wakeup{ item_wakeup_kind::ready_check, calendar::turn_zero + 30_minutes },
        };
    };

    clear_test_enumerate_handlers();
    register_test_enumerate_handler( itype_test_wakeup_dummy, enumerator );

    item_wakeup_manager &mgr = get_item_wakeups();
    mgr.clear();

    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    item &on_map = here.add_item( origin, item( itype_test_wakeup_dummy, calendar::turn ) );
    const int64_t uid = on_map.uid().get_value();

    mgr.clear();
    REQUIRE_FALSE( mgr.is_scheduled( uid, item_wakeup_kind::ready_check ) );

    here.shift( point_rel_sm::east );

    CHECK( mgr.is_scheduled( uid, item_wakeup_kind::ready_check ) );

    mgr.clear();
    clear_test_enumerate_handlers();
}

TEST_CASE( "item_wakeup_map_load_recurses_into_containers",
           "[item_wakeup][reconcile][container]" )
{
    auto enumerator = []( const item & /*it*/, const item_location & /*loc*/ )
    -> std::vector<desired_wakeup> {
        return {
            desired_wakeup{ item_wakeup_kind::ready_check, calendar::turn_zero + 30_minutes },
        };
    };

    clear_test_enumerate_handlers();
    register_test_enumerate_handler( itype_test_wakeup_dummy, enumerator );

    item_wakeup_manager &mgr = get_item_wakeups();
    mgr.clear();

    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item box( itype_backpack, calendar::turn );
    REQUIRE( box.put_in( item( itype_test_wakeup_dummy, calendar::turn ),
                         pocket_type::CONTAINER ).success() );
    item &placed_box = here.add_item( origin, box );
    REQUIRE( !placed_box.all_items_top().empty() );
    const int64_t inner_uid = placed_box.all_items_top().front()->uid().get_value();

    mgr.clear();
    here.load( here.get_abs_sub(), /*update_vehicles=*/false, /*pump_events=*/false );

    CHECK( mgr.is_scheduled( inner_uid, item_wakeup_kind::ready_check ) );

    mgr.clear();
    clear_test_enumerate_handlers();
}
