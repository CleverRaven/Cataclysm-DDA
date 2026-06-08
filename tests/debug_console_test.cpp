#include <cstddef>
#include <functional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cata_catch.h"
#include "debug_actions_table.h"
#include "debug_console.h"
#include "debug_menu_types.h"
#include "debug_monitor_targets.h"
#include "flexbuffer_json.h"
#include "json.h"
#include "json_loader.h"

TEST_CASE( "fuzzy_score_basics", "[debug_console]" )
{
    SECTION( "empty needle returns non-zero" ) {
        CHECK( debug_menu::fuzzy_score( "anything", "" ) > 0 );
    }
    SECTION( "no match returns zero" ) {
        CHECK( debug_menu::fuzzy_score( "abc", "xyz" ) == 0 );
    }
    SECTION( "exact substring scores higher than scattered" ) {
        const int contiguous = debug_menu::fuzzy_score( "spawn item group", "spawn" );
        const int scattered = debug_menu::fuzzy_score( "splice paint nasal", "spawn" );
        CHECK( contiguous > scattered );
    }
    SECTION( "case insensitive" ) {
        CHECK( debug_menu::fuzzy_score( "SPAWN MONSTER", "spawn" ) > 0 );
        CHECK( debug_menu::fuzzy_score( "spawn monster", "SPAWN" ) > 0 );
    }
    SECTION( "prefix bonus -- match at start outscores match in middle" ) {
        const int prefix = debug_menu::fuzzy_score( "spawn whatever", "spawn" );
        const int middle = debug_menu::fuzzy_score( "test spawn here", "spawn" );
        CHECK( prefix > middle );
    }
    SECTION( "longer contiguous run scores higher" ) {
        const int run3 = debug_menu::fuzzy_score( "abc xyz", "abc" );
        const int run1 = debug_menu::fuzzy_score( "a b c xyz", "abc" );
        CHECK( run3 > run1 );
    }
}

TEST_CASE( "all_actions_table_invariants", "[debug_console]" )
{
    const auto &table = debug_menu::all_actions();
    REQUIRE( !table.empty() );

    SECTION( "no duplicate debug_menu_index entries" ) {
        std::unordered_set<int> seen;
        for( const debug_menu::debug_action_entry &e : table ) {
            const int id = static_cast<int>( e.id );
            INFO( "duplicate entry for id=" << id << " label=" << e.label );
            CHECK( seen.insert( id ).second );
        }
    }
    SECTION( "all entries have non-empty label + category + search keys" ) {
        for( const debug_menu::debug_action_entry &e : table ) {
            INFO( "label=" << ( e.label ? e.label : "(null)" ) );
            REQUIRE( e.label != nullptr );
            CHECK( std::string( e.label ).size() > 0 );
            REQUIRE( e.category != nullptr );
            CHECK( std::string( e.category ).size() > 0 );
            REQUIRE( e.search_keys != nullptr );
            CHECK( std::string( e.search_keys ).size() > 0 );
        }
    }
    SECTION( "table covers every debug_menu_index value" ) {
        const size_t enum_count = static_cast<size_t>( debug_menu::debug_menu_index::last );
        CHECK( table.size() == enum_count );
        std::unordered_set<int> seen;
        for( const debug_menu::debug_action_entry &e : table ) {
            seen.insert( static_cast<int>( e.id ) );
        }
        for( int i = 0; i < static_cast<int>( enum_count ); i++ ) {
            INFO( "missing table entry for debug_menu_index value " << i );
            CHECK( seen.count( i ) == 1 );
        }
    }
}

TEST_CASE( "tab_registry_invariants", "[debug_console]" )
{
    const std::vector<debug_menu::tab_registry_view_entry> reg =
        debug_menu::tab_registry_snapshot();
    REQUIRE( !reg.empty() );

    SECTION( "every id_string is distinct and non-empty" ) {
        std::unordered_set<std::string> seen;
        for( const debug_menu::tab_registry_view_entry &e : reg ) {
            const std::string id( e.id );
            INFO( "tab id duplicated or empty: '" << id << "'" );
            CHECK( !id.empty() );
            CHECK( seen.insert( id ).second );
        }
    }
    SECTION( "no category is claimed by more than one tab" ) {
        std::unordered_set<std::string> seen;
        for( const debug_menu::tab_registry_view_entry &e : reg ) {
            for( std::string_view c : e.categories ) {
                const std::string key( c );
                INFO( "category claimed by multiple tabs: " << key );
                CHECK( seen.insert( key ).second );
            }
        }
    }
    SECTION( "every action category is claimed by exactly one tab" ) {
        const auto &table = debug_menu::all_actions();
        REQUIRE( !table.empty() );
        std::unordered_set<std::string> distinct_cats;
        for( const debug_menu::debug_action_entry &e : table ) {
            distinct_cats.insert( e.category );
        }
        for( const std::string &cat : distinct_cats ) {
            int matches = 0;
            for( const debug_menu::tab_registry_view_entry &e : reg ) {
                for( std::string_view c : e.categories ) {
                    if( c == cat ) {
                        ++matches;
                    }
                }
            }
            INFO( "action category '" << cat << "' claimed by " << matches << " tabs" );
            CHECK( matches == 1 );
        }
    }
    SECTION( "every declared category is used by at least one action" ) {
        const auto &table = debug_menu::all_actions();
        std::unordered_set<std::string> distinct_cats;
        for( const debug_menu::debug_action_entry &e : table ) {
            distinct_cats.insert( e.category );
        }
        for( const debug_menu::tab_registry_view_entry &e : reg ) {
            for( std::string_view c : e.categories ) {
                const std::string key( c );
                INFO( "registry category never claimed by any action: " << key );
                CHECK( distinct_cats.count( key ) == 1 );
            }
        }
    }
}

TEST_CASE( "all_monitor_targets_table_invariants", "[debug_console]" )
{
    const auto &targets = debug_menu::all_monitor_targets();
    REQUIRE( !targets.empty() );

    SECTION( "every entry has non-null label/hint/factory" ) {
        for( const debug_menu::monitor_target_entry &t : targets ) {
            REQUIRE( t.label != nullptr );
            CHECK( std::string( t.label ).size() > 0 );
            REQUIRE( t.hint != nullptr );
            REQUIRE( t.make_snap != nullptr );
        }
    }
    SECTION( "no duplicate labels" ) {
        std::unordered_set<std::string> seen;
        for( const debug_menu::monitor_target_entry &t : targets ) {
            INFO( "duplicate label: " << t.label );
            CHECK( seen.insert( t.label ).second );
        }
    }
}

// Per-tab persistence: load_state -> save_state shape preservation.

namespace
{

JsonObject json_object_from( const std::string &s )
{
    JsonObject obj = json_loader::from_string( s ).get_object();
    obj.allow_omitted_members();
    return obj;
}

JsonObject save_to_object( const debug_menu::console_tab_view &tab )
{
    std::ostringstream os;
    JsonOut jw( os );
    jw.start_object();
    tab.save_state( jw );
    jw.end_object();
    return json_object_from( os.str() );
}

} // namespace

TEST_CASE( "tab_creatures_view_persistence_roundtrip", "[debug_console][persistence]" )
{
    SECTION( "nested-only load preserves every field on save" ) {
        debug_menu::tab_creatures_view tab;
        tab.load_state(
            json_object_from( R"({"filter":"hostile","selected_id":"npc_42","filter_within_bubble":false})" ) );
        JsonObject out = save_to_object( tab );
        std::string filter;
        std::string selected_id;
        bool within = true;
        CHECK( out.read( "filter", filter ) );
        CHECK( filter == "hostile" );
        CHECK( out.read( "selected_id", selected_id ) );
        CHECK( selected_id == "npc_42" );
        CHECK( out.read( "filter_within_bubble", within ) );
        CHECK_FALSE( within );
    }
    SECTION( "filter_within_bubble default stays true on empty input" ) {
        // Pins emit-on-default: empty input -> field present with true.
        // Relax to "absent or true" if save_state ever omits defaults.
        debug_menu::tab_creatures_view tab;
        tab.load_state( json_object_from( "{}" ) );
        JsonObject out = save_to_object( tab );
        bool within = false;
        CHECK( out.read( "filter_within_bubble", within ) );
        CHECK( within );
    }
}

TEST_CASE( "tab_items_view_persistence_roundtrip", "[debug_console][persistence]" )
{
    SECTION( "nested-only load round-trips filter, source, typed selection" ) {
        debug_menu::tab_items_view tab;
        tab.load_state(
            json_object_from(
                R"({"filter":"food","source":"npc:7","selected_source":"npc:7",)"
                R"("selected_type":"can_food","selected_location":"backpack","selected_rank":2})" ) );
        JsonObject out = save_to_object( tab );
        std::string filter;
        std::string source;
        std::string sel_src;
        std::string sel_type;
        std::string sel_loc;
        int sel_rank = -1;
        CHECK( out.read( "filter", filter ) );
        CHECK( filter == "food" );
        CHECK( out.read( "source", source ) );
        CHECK( source == "npc:7" );
        CHECK( out.read( "selected_source", sel_src ) );
        CHECK( sel_src == "npc:7" );
        CHECK( out.read( "selected_type", sel_type ) );
        CHECK( sel_type == "can_food" );
        CHECK( out.read( "selected_location", sel_loc ) );
        CHECK( sel_loc == "backpack" );
        CHECK( out.read( "selected_rank", sel_rank ) );
        CHECK( sel_rank == 2 );
    }
}

TEST_CASE( "tab_eoc_view_persistence_roundtrip", "[debug_console][persistence]" )
{
    SECTION( "nested-only load preserves every field on save" ) {
        debug_menu::tab_eoc_view tab;
        tab.load_state(
            json_object_from( R"({"filter":"npc_","type_filter":2,"selected_id":"some_eoc"})" ) );
        JsonObject out = save_to_object( tab );
        std::string filter;
        std::string sel;
        int type = -1;
        CHECK( out.read( "filter", filter ) );
        CHECK( filter == "npc_" );
        CHECK( out.read( "type_filter", type ) );
        CHECK( type == 2 );
        CHECK( out.read( "selected_id", sel ) );
        CHECK( sel == "some_eoc" );
    }
}

TEST_CASE( "tab_data_view_persistence_roundtrip", "[debug_console][persistence]" )
{
    SECTION( "nested-only load preserves field on save" ) {
        debug_menu::tab_data_view tab;
        tab.load_state( json_object_from( R"({"var_filter":"npc_hostile"})" ) );
        JsonObject out = save_to_object( tab );
        std::string filter;
        CHECK( out.read( "var_filter", filter ) );
        CHECK( filter == "npc_hostile" );
    }
}

TEST_CASE( "tab_tiles_view_persistence_roundtrip", "[debug_console][persistence]" )
{
    SECTION( "nested-only load preserves field on save" ) {
        debug_menu::tab_tiles_view tab;
        tab.load_state( json_object_from( R"({"tile_coord_input":"1,2,3"})" ) );
        JsonObject out = save_to_object( tab );
        std::string coord;
        CHECK( out.read( "tile_coord_input", coord ) );
        CHECK( coord == "1,2,3" );
    }
}
