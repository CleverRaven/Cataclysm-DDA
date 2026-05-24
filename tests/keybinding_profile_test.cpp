#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "cata_catch.h"
#include "cata_path.h"
#include "flexbuffer_json.h"
#include "json.h"
#include "json_loader.h"
#include "path_info.h"

namespace
{

// (input_method, key, sorted modifiers) — a binding's identity within a category.
using binding_key_t = std::tuple<std::string, std::string, std::vector<std::string>>;

struct profile_entry {
    std::string id;
    std::string category;
    std::vector<std::pair<binding_key_t, bool>> bindings;  // bool: has modifiers
};

static std::vector<profile_entry> parse_profile( const cata_path &file_name )
{
    std::vector<profile_entry> out;
    std::optional<JsonValue> jsin_opt = json_loader::from_path_opt( file_name );
    REQUIRE( jsin_opt.has_value() );
    JsonArray arr = *jsin_opt;
    for( JsonObject obj : arr ) {
        profile_entry pe;
        // CDDA's JSON loader fails if any field is left unread; touch the
        // metadata fields here even though we don't otherwise need them.
        ( void ) obj.get_string( "type", "keybinding" );
        if( obj.has_member( "name" ) ) {
            ( void ) obj.get_string( "name" );
        }
        pe.id = obj.get_string( "id" );
        pe.category = obj.get_string( "category", "default" );
        if( obj.has_member( "bindings" ) ) {
            for( JsonObject b : obj.get_array( "bindings" ) ) {
                std::string method = b.get_string( "input_method" );
                std::string key = b.get_string( "key" );
                std::vector<std::string> mods;
                if( b.has_member( "mod" ) ) {
                    for( const std::string mod : b.get_array( "mod" ) ) {
                        mods.push_back( mod );
                    }
                    std::sort( mods.begin(), mods.end() );
                }
                pe.bindings.emplace_back( binding_key_t{ method, key, mods }, !mods.empty() );
            }
        }
        out.push_back( std::move( pe ) );
    }
    return out;
}

static void validate_profile( const std::vector<profile_entry> &entries,
                              const std::string &profile_label )
{
    INFO( "profile: " << profile_label );

    // 1. No duplicate (category, id) — each action listed at most once.
    std::set<std::pair<std::string, std::string>> seen_keys;
    for( const profile_entry &e : entries ) {
        const auto k = std::make_pair( e.category, e.id );
        INFO( "duplicate (category, id): " << e.category << "/" << e.id );
        CHECK( seen_keys.insert( k ).second );
    }

    // 2. No two distinct actions in the same category share a binding.
    std::map<std::pair<std::string, binding_key_t>, std::string> owners;
    for( const profile_entry &e : entries ) {
        for( const auto &kv : e.bindings ) {
            const binding_key_t &bk = kv.first;
            auto map_key = std::make_pair( e.category, bk );
            auto it = owners.find( map_key );
            INFO( "binding collision in " << e.category << ": " << std::get<0>( bk )
                  << "/" << std::get<1>( bk ) << " between "
                  << ( it != owners.end() ? it->second : "" ) << " and " << e.id );
            CHECK( ( it == owners.end() || it->second == e.id ) );
            owners[map_key] = e.id;
        }
    }

    // 3. Informational: report DEFAULTMODE actions whose only bindings carry a
    // modifier.  Such bindings cannot match on platforms where modifier state
    // is dropped (ncurses; Android without ANDROID_KEYCODE_MODE).  Modifier-
    // variant actions (SNEAK_*, RUN_*, PEEK_*) are intentionally modifier-only
    // and skipped.  Other entries here suggest a curses-unreachable action;
    // not a hard failure (this profile is desktop-tiles-first by design),
    // logged so doc/USER_INTERFACE_AND_ACCESSIBILITY.md stays accurate.
    auto is_modifier_variant = []( const std::string & id ) {
        return id.rfind( "SNEAK_", 0 ) == 0 || id.rfind( "RUN_", 0 ) == 0 ||
               id.rfind( "PEEK_", 0 ) == 0;
    };
    int modifier_only = 0;
    for( const profile_entry &e : entries ) {
        if( e.category != "DEFAULTMODE" || e.bindings.empty() ||
            is_modifier_variant( e.id ) ) {
            continue;
        }
        bool has_unmod = false;
        for( const auto &kv : e.bindings ) {
            if( !kv.second ) {
                has_unmod = true;
                break;
            }
        }
        if( !has_unmod ) {
            ++modifier_only;
            UNSCOPED_INFO( "modifier-only DEFAULTMODE action (curses-unreachable): "
                           << e.id );
        }
    }
    UNSCOPED_INFO( profile_label << ": modifier-only DEFAULTMODE actions: "
                   << modifier_only );
}

} // namespace

TEST_CASE( "default-numpad_profile_structure", "[keybinding_profile]" )
{
    const std::vector<profile_entry> entries =
        parse_profile( PATH_INFO::keybindings_profile( "default-numpad" ) );
    REQUIRE( !entries.empty() );
    validate_profile( entries, "default-numpad" );
}

TEST_CASE( "default-laptop_profile_structure", "[keybinding_profile]" )
{
    const std::vector<profile_entry> entries =
        parse_profile( PATH_INFO::keybindings_profile( "default-laptop" ) );
    REQUIRE( !entries.empty() );
    validate_profile( entries, "default-laptop" );
}

TEST_CASE( "keybindings_base_layer_parses", "[keybinding_profile]" )
{
    // base.json must always exist and be a valid (possibly empty) array of
    // keybinding entries.
    const std::vector<profile_entry> entries = parse_profile( PATH_INFO::keybindings_base() );
    // Empty base is acceptable (current state); when populated, each entry
    // must still pass the structural checks.
    if( !entries.empty() ) {
        validate_profile( entries, "base" );
    }
}
