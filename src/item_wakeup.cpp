#include "item_wakeup.h"

#include <algorithm>
#include <map>
#include <queue>
#include <string>
#include <utility>

#include "cata_assert.h"
#include "character_id.h"
#include "debug.h"
#include "flexbuffer_json.h"
#include "item.h"
#include "item_location.h"
#include "item_uid.h"
#include "json.h"
#include "type_id.h"

namespace
{

constexpr int CURRENT_VERSION = 1;

// Stale dispatches before a single noise-control debugmsg fires.
// Reset by clear() and deserialize().
constexpr int STALE_DEBUGMSG_THRESHOLD = 32;

const char *kind_to_string( item_wakeup_kind k )
{
    switch( k ) {
        case item_wakeup_kind::alarm:
            return "alarm";
        case item_wakeup_kind::ready_check:
            return "ready_check";
        case item_wakeup_kind::fail_check:
            return "fail_check";
        case item_wakeup_kind::last:
            break;
    }
    return "invalid";
}

bool string_to_kind( const std::string &s, item_wakeup_kind &out )
{
    if( s == "alarm" ) {
        out = item_wakeup_kind::alarm;
        return true;
    }
    if( s == "ready_check" ) {
        out = item_wakeup_kind::ready_check;
        return true;
    }
    if( s == "fail_check" ) {
        out = item_wakeup_kind::fail_check;
        return true;
    }
    return false;
}

const char *place_to_string( item_locator_hint::place p )
{
    switch( p ) {
        case item_locator_hint::place::map:
            return "map";
        case item_locator_hint::place::vehicle:
            return "vehicle";
        case item_locator_hint::place::character:
            return "character";
        case item_locator_hint::place::unknown:
            break;
    }
    return "unknown";
}

bool string_to_place( const std::string &s, item_locator_hint::place &out )
{
    if( s == "map" ) {
        out = item_locator_hint::place::map;
        return true;
    }
    if( s == "vehicle" ) {
        out = item_locator_hint::place::vehicle;
        return true;
    }
    if( s == "character" ) {
        out = item_locator_hint::place::character;
        return true;
    }
    if( s == "unknown" ) {
        out = item_locator_hint::place::unknown;
        return true;
    }
    return false;
}

void serialize_hint( JsonOut &jsout, const item_locator_hint &hint )
{
    jsout.start_object();
    jsout.member( "where", place_to_string( hint.where ) );
    if( hint.where == item_locator_hint::place::map ) {
        const tripoint_abs_ms *p = std::get_if<tripoint_abs_ms>( &hint.location );
        if( p != nullptr ) {
            jsout.member( "abs_ms", *p );
        }
    } else if( hint.where == item_locator_hint::place::vehicle ) {
        const vehicle_hint *v = std::get_if<vehicle_hint>( &hint.location );
        if( v != nullptr ) {
            jsout.member( "cargo_square", v->cargo_square );
            jsout.member( "part_index", v->part_index );
            jsout.member( "mount_offset", v->mount_offset );
        }
    } else if( hint.where == item_locator_hint::place::character ) {
        const character_id *c = std::get_if<character_id>( &hint.location );
        if( c != nullptr ) {
            jsout.member( "character", c->get_value() );
        }
    }
    jsout.end_object();
}

bool deserialize_hint( const JsonObject &jo, item_locator_hint &hint )
{
    std::string where_str;
    if( !jo.read( "where", where_str ) ) {
        return false;
    }
    if( !string_to_place( where_str, hint.where ) ) {
        return false;
    }
    if( hint.where == item_locator_hint::place::map ) {
        tripoint_abs_ms p;
        if( !jo.read( "abs_ms", p ) ) {
            hint.where = item_locator_hint::place::unknown;
            hint.location = std::monostate{};
            return true;
        }
        hint.location = p;
    } else if( hint.where == item_locator_hint::place::vehicle ) {
        vehicle_hint v;
        if( !jo.read( "cargo_square", v.cargo_square ) ) {
            hint.where = item_locator_hint::place::unknown;
            hint.location = std::monostate{};
            return true;
        }
        jo.read( "part_index", v.part_index );
        jo.read( "mount_offset", v.mount_offset );
        hint.location = v;
    } else if( hint.where == item_locator_hint::place::character ) {
        int64_t cid = 0;
        if( !jo.read( "character", cid ) ) {
            hint.where = item_locator_hint::place::unknown;
            hint.location = std::monostate{};
            return true;
        }
        hint.location = character_id( static_cast<int>( cid ) );
    } else {
        hint.location = std::monostate{};
    }
    return true;
}

std::map<itype_id, item_wakeup_test_handler> &test_handler_registry()
{
    static std::map<itype_id, item_wakeup_test_handler> registry;
    return registry;
}

std::map<itype_id, item_wakeup_test_enumerator> &test_enumerator_registry()
{
    static std::map<itype_id, item_wakeup_test_enumerator> registry;
    return registry;
}

bool processing_active = false;

}  // namespace

void register_test_wakeup_handler( const itype_id &id, item_wakeup_test_handler fn )
{
    test_handler_registry()[id] = fn;
}

void clear_test_wakeup_handlers()
{
    test_handler_registry().clear();
}

void register_test_enumerate_handler( const itype_id &id, item_wakeup_test_enumerator fn )
{
    test_enumerator_registry()[id] = fn;
}

void clear_test_enumerate_handlers()
{
    test_enumerator_registry().clear();
}

std::vector<desired_wakeup> enumerate_scheduled_dispatch( const item &it )
{
    const std::map<itype_id, item_wakeup_test_enumerator> &reg = test_enumerator_registry();
    const auto entry = reg.find( it.typeId() );
    if( entry != reg.end() ) {
        return entry->second( it );
    }
    return {};
}

// Dispatcher called from item::actualize_scheduled.
static void dispatch_actualize( item &it, item_wakeup_kind kind, time_point now )
{
    const std::map<itype_id, item_wakeup_test_handler> &reg = test_handler_registry();
    const auto it_handler = reg.find( it.typeId() );
    if( it_handler != reg.end() ) {
        it_handler->second( it, kind, now );
    }
}

// Min-heap comparator: top is smallest by (when, uid, kind).  push_heap and
// friends produce a max-heap, so this returns true when `a` should sink
// below `b`, i.e. `a` is greater.
bool item_wakeup_manager::heap_greater( const entry_iter &a, const entry_iter &b )
{
    if( a->when != b->when ) {
        return a->when > b->when;
    }
    if( a->uid != b->uid ) {
        return a->uid > b->uid;
    }
    return static_cast<int>( a->kind ) > static_cast<int>( b->kind );
}

bool item_wakeup_manager::is_live( const entry_iter &it ) const
{
    const auto map_it = by_key_.find( {it->uid, it->kind} );
    return map_it != by_key_.end() && map_it->second == it;
}

void item_wakeup_manager::clean_heap_top()
{
    while( !heap_.empty() && !is_live( heap_.front() ) ) {
        const entry_iter top = heap_.front();
        std::pop_heap( heap_.begin(), heap_.end(), heap_greater );
        heap_.pop_back();
        entries_.erase( top );
    }
}

void item_wakeup_manager::schedule_or_update( int64_t uid, time_point when,
        item_wakeup_kind kind, item_locator_hint hint )
{
    entry e;
    e.uid = uid;
    e.kind = kind;
    e.when = when;
    e.hint = hint;
    const entry_iter new_it = entries_.insert( e );
    by_key_[ {uid, kind}] = new_it;
    heap_.push_back( new_it );
    std::push_heap( heap_.begin(), heap_.end(), heap_greater );
    clean_heap_top();
}

void item_wakeup_manager::cancel( int64_t uid, item_wakeup_kind kind )
{
    by_key_.erase( {uid, kind} );
    clean_heap_top();
}

void item_wakeup_manager::cancel_all( int64_t uid )
{
    for( int k = 0; k < static_cast<int>( item_wakeup_kind::last ); ++k ) {
        by_key_.erase( {uid, static_cast<item_wakeup_kind>( k )} );
    }
    clean_heap_top();
}

void item_wakeup_manager::rebuild_for_item( item &it, item_locator_hint hint )
{
    const int64_t uid = it.uid().get_value();
    const std::vector<desired_wakeup> desired = it.enumerate_scheduled_wakeups();

    for( int k = 0; k < static_cast<int>( item_wakeup_kind::last ); ++k ) {
        const item_wakeup_kind kind = static_cast<item_wakeup_kind>( k );
        const bool wanted = std::any_of( desired.begin(), desired.end(),
        [kind]( const desired_wakeup & d ) {
            return d.kind == kind;
        } );
        if( !wanted ) {
            by_key_.erase( {uid, kind} );
        }
    }
    for( const desired_wakeup &d : desired ) {
        schedule_or_update( uid, d.when, d.kind, hint );
    }
    clean_heap_top();
}

bool item_wakeup_manager::is_scheduled( int64_t uid, item_wakeup_kind kind ) const
{
    return by_key_.count( {uid, kind} ) > 0;
}

std::optional<time_point> item_wakeup_manager::get( int64_t uid,
        item_wakeup_kind kind ) const
{
    const auto map_it = by_key_.find( {uid, kind} );
    if( map_it == by_key_.end() ) {
        return std::nullopt;
    }
    return map_it->second->when;
}

std::optional<scheduled_wakeup_info> item_wakeup_manager::peek_next_wakeup() const
{
    if( heap_.empty() ) {
        return std::nullopt;
    }
    const entry_iter top = heap_.front();
    scheduled_wakeup_info info;
    info.uid = top->uid;
    info.kind = top->kind;
    info.when = top->when;
    info.hint = top->hint;
    return info;
}

std::vector<scheduled_wakeup_info> item_wakeup_manager::dump() const
{
    std::vector<scheduled_wakeup_info> out;
    out.reserve( by_key_.size() );
    for( const auto &kv : by_key_ ) {
        const entry_iter it = kv.second;
        scheduled_wakeup_info info;
        info.uid = it->uid;
        info.kind = it->kind;
        info.when = it->when;
        info.hint = it->hint;
        out.push_back( info );
    }
    std::sort( out.begin(), out.end(),
    []( const scheduled_wakeup_info & a, const scheduled_wakeup_info & b ) {
        if( a.when != b.when ) {
            return a.when < b.when;
        }
        if( a.uid != b.uid ) {
            return a.uid < b.uid;
        }
        return static_cast<int>( a.kind ) < static_cast<int>( b.kind );
    } );
    return out;
}

item_wakeup_manager::stats item_wakeup_manager::get_stats() const
{
    stats s = stats_;
    s.total_pending = static_cast<int>( by_key_.size() );
    return s;
}

void item_wakeup_manager::process( time_point now )
{
    cata_assert( !processing_active );
    processing_active = true;

    std::vector<entry> snapshot;
    while( !heap_.empty() ) {
        const entry_iter top = heap_.front();
        if( !is_live( top ) ) {
            std::pop_heap( heap_.begin(), heap_.end(), heap_greater );
            heap_.pop_back();
            entries_.erase( top );
            continue;
        }
        if( top->when > now ) {
            break;
        }
        snapshot.push_back( *top );
        std::pop_heap( heap_.begin(), heap_.end(), heap_greater );
        heap_.pop_back();
        by_key_.erase( {top->uid, top->kind} );
        entries_.erase( top );
    }

    for( const entry &e : snapshot ) {
        item_location loc = find_item_by_uid( e.uid, e.hint );
        if( !loc ) {
            stats_.stale_resolution++;
            stale_seen_uids_++;
            if( stale_seen_uids_ >= STALE_DEBUGMSG_THRESHOLD && !stale_msg_emitted_ ) {
                debugmsg( "item_wakeup_manager: %d stale uids unresolved",
                          stale_seen_uids_ );
                stale_msg_emitted_ = true;
            }
            continue;
        }
        loc->actualize_scheduled( e.kind, now );
    }

    clean_heap_top();
    processing_active = false;
}

void item_wakeup_manager::clear()
{
    entries_.clear();
    by_key_.clear();
    heap_.clear();
    stats_ = stats{};
    stale_seen_uids_ = 0;
    stale_msg_emitted_ = false;
}

void item_wakeup_manager::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "version", CURRENT_VERSION );
    jsout.member( "wakeups" );
    jsout.start_array();
    std::vector<entry_iter> live;
    live.reserve( by_key_.size() );
    for( const std::pair<const key_t, entry_iter> &kv : by_key_ ) {
        live.push_back( kv.second );
    }
    std::sort( live.begin(), live.end(),
    []( const entry_iter & a, const entry_iter & b ) {
        if( a->when != b->when ) {
            return a->when < b->when;
        }
        if( a->uid != b->uid ) {
            return a->uid < b->uid;
        }
        return static_cast<int>( a->kind ) < static_cast<int>( b->kind );
    } );
    cata_assert( entries_.size() >= live.size() );
    for( const entry_iter &it : live ) {
        const entry &e = *it;
        jsout.start_object();
        jsout.member( "uid", e.uid );
        jsout.member( "kind", kind_to_string( e.kind ) );
        jsout.member( "when", e.when );
        jsout.member( "hint" );
        serialize_hint( jsout, e.hint );
        jsout.end_object();
    }
    jsout.end_array();
    jsout.end_object();
}

void item_wakeup_manager::deserialize( const JsonObject &jo )
{
    clear();

    int version = 0;
    if( !jo.read( "version", version ) ) {
        debugmsg( "item_wakeup_manager: save without version, treating as v1" );
    } else if( version != CURRENT_VERSION ) {
        debugmsg( "item_wakeup_manager: unknown save version %d, dropping queue",
                  version );
        return;
    }

    if( !jo.has_array( "wakeups" ) ) {
        return;
    }

    std::map<std::pair<int64_t, item_wakeup_kind>, entry> dedup;
    int dropped = 0;
    for( JsonObject row : jo.get_array( "wakeups" ) ) {
        row.allow_omitted_members();
        int64_t uid = 0;
        std::string kind_str;
        time_point when;
        item_wakeup_kind kind = item_wakeup_kind::alarm;
        const bool got_uid = row.read( "uid", uid );
        const bool got_kind = row.read( "kind", kind_str );
        const bool got_when = row.read( "when", when );
        item_locator_hint hint;
        if( row.has_object( "hint" ) ) {
            JsonObject hint_obj = row.get_object( "hint" );
            if( !deserialize_hint( hint_obj, hint ) ) {
                hint = item_locator_hint{};
            }
        }
        if( !got_uid || uid <= 0 || !got_kind || !string_to_kind( kind_str, kind )
            || !got_when ) {
            dropped++;
            continue;
        }
        entry e;
        e.uid = uid;
        e.kind = kind;
        e.when = when;
        e.hint = hint;
        const std::pair<int64_t, item_wakeup_kind> key{ uid, kind };
        const std::map<std::pair<int64_t, item_wakeup_kind>, entry>::const_iterator existing
            = dedup.find( key );
        if( existing == dedup.end() ) {
            dedup[key] = e;
        } else {
            stats_.duplicate_event++;
            if( existing->second.when < when ) {
                dedup[key] = e;
            }
        }
    }
    for( const auto &kv : dedup ) {
        const entry_iter it = entries_.insert( kv.second );
        by_key_[kv.first] = it;
        heap_.push_back( it );
    }
    std::make_heap( heap_.begin(), heap_.end(), heap_greater );
    stats_.dropped_on_load = dropped;
}

// Item-side dispatch entry called from item.cpp.
void actualize_scheduled_dispatch( item &it, item_wakeup_kind kind, time_point now )
{
    dispatch_actualize( it, kind, now );
}
