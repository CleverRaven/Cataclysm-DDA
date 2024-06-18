#include "mapbuffer.h"

#include <chrono>
#include <exception>
#include <filesystem>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "cata_utility.h"
#include "debug.h"
#include "filesystem.h"
#include "input.h"
#include "json.h"
#include "map.h"
#include "output.h"
#include "overmapbuffer.h"
#include "path_info.h"
#include "popup.h"
#include "string_formatter.h"
#include "submap.h"
#include "translations.h"
#include "ui_manager.h"

#define dbg(x) DebugLog((x),D_MAP) << __FILE__ << ":" << __LINE__ << ": "

class game;
// NOLINTNEXTLINE(cata-static-declarations)
extern std::unique_ptr<game> g;
// NOLINTNEXTLINE(cata-static-declarations)
extern const int savegame_version;

static cata_path find_quad_path( const cata_path &dirname, const tripoint_abs_omt &om_addr )
{
    return dirname / string_format( "%d.%d.%d.map", om_addr.x(), om_addr.y(), om_addr.z() );
}

static cata_path find_dirname( const tripoint_abs_omt &om_addr )
{
    const tripoint_abs_seg segment_addr = project_to<coords::seg>( om_addr );
    return PATH_INFO::world_base_save_path_path() / "maps" / string_format( "%d.%d.%d",
            segment_addr.x(),
            segment_addr.y(), segment_addr.z() );
}

mapbuffer MAPBUFFER;

mapbuffer::mapbuffer() = default;
mapbuffer::~mapbuffer() = default;

void mapbuffer::clear()
{
    submaps.clear();
}

void mapbuffer::clear_outside_reality_bubble()
{
    map &here = get_map();
    auto it = submaps.begin();
    while( it != submaps.end() ) {
        if( here.inbounds( it->first ) ) {
            ++it;
        } else {
            it = submaps.erase( it );
        }
    }
}

bool mapbuffer::add_submap( const tripoint_abs_sm &p, std::unique_ptr<submap> &sm )
{
    if( submaps.count( p ) ) {
        return false;
    }

    submaps[p] = std::move( sm );

    return true;
}

bool mapbuffer::add_submap( const tripoint_abs_sm &p, submap *sm )
{
    // FIXME: get rid of this overload and make submap ownership semantics sane.
    std::unique_ptr<submap> temp( sm );
    bool result = add_submap( p, temp );
    if( !result ) {
        // NOLINTNEXTLINE( bugprone-unused-return-value )
        temp.release();
    }
    return result;
}

void mapbuffer::remove_submap( const tripoint_abs_sm &addr )
{
    auto m_target = submaps.find( addr );
    if( m_target == submaps.end() ) {
        debugmsg( "Tried to remove non-existing submap %s", addr.to_string() );
        return;
    }
    submaps.erase( m_target );
}

submap *mapbuffer::lookup_submap( const tripoint_abs_sm &p )
{
    dbg( D_INFO ) << "mapbuffer::lookup_submap( x[" << p.x() << "], y[" << p.y() << "], z["
                  << p.z() << "])";

    const auto iter = submaps.find( p );
    if( iter == submaps.end() ) {
        try {
            return unserialize_submaps( p );
        } catch( const std::exception &err ) {
            debugmsg( "Failed to load submap %s: %s", p.to_string(), err.what() );
        }
        return nullptr;
    }

    return iter->second.get();
}

void mapbuffer::save( bool delete_after_save )
{
    assure_dir_exist( PATH_INFO::world_base_save_path() + "/maps" );

    int num_saved_submaps = 0;
    int num_total_submaps = submaps.size();

    map &here = get_map();

    static_popup popup;

    // A set of already-saved submaps, in global overmap coordinates.
    std::set<tripoint_abs_omt> saved_submaps;
    std::list<tripoint_abs_sm> submaps_to_delete;
    static constexpr std::chrono::milliseconds update_interval( 500 );
    std::chrono::steady_clock::time_point last_update = std::chrono::steady_clock::now();

    for( auto &elem : submaps ) {
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        if( last_update + update_interval < now ) {
            popup.message( _( "Please wait as the map saves [%d/%d]" ),
                           num_saved_submaps, num_total_submaps );
            ui_manager::redraw();
            refresh_display();
            inp_mngr.pump_events();
            last_update = now;
        }
        // Whatever the coordinates of the current submap are,
        // we're saving a 2x2 quad of submaps at a time.
        // Submaps are generated in quads, so we know if we have one member of a quad,
        // we have the rest of it, if that assumption is broken we have REAL problems.
        const tripoint_abs_omt om_addr = project_to<coords::omt>( elem.first );
        if( saved_submaps.count( om_addr ) != 0 ) {
            // Already handled this one.
            continue;
        }
        saved_submaps.insert( om_addr );

        // A segment is a chunk of 32x32 submap quads.
        // We're breaking them into subdirectories so there aren't too many files per directory.
        // Might want to make a set for this one too so it's only checked once per save().
        const cata_path dirname = find_dirname( om_addr );
        const cata_path quad_path = find_quad_path( dirname, om_addr );

        bool inside_reality_bubble = here.inbounds( om_addr );
        // delete_on_save deletes everything, otherwise delete submaps
        // outside the current map.
        save_quad( dirname, quad_path, om_addr, submaps_to_delete,
                   delete_after_save || !inside_reality_bubble );
        num_saved_submaps += 4;
    }
    for( auto &elem : submaps_to_delete ) {
        remove_submap( elem );
    }
}

void mapbuffer::save_quad(
    const cata_path &dirname, const cata_path &filename, const tripoint_abs_omt &om_addr,
    std::list<tripoint_abs_sm> &submaps_to_delete, bool delete_after_save )
{
    std::vector<point> offsets;
    std::vector<tripoint_abs_sm> submap_addrs;
    offsets.push_back( point_zero );
    offsets.push_back( point_south );
    offsets.push_back( point_east );
    offsets.push_back( point_south_east );

    bool all_uniform = true;
    bool reverted_to_uniform = false;
    bool const file_exists = fs::exists( filename.get_unrelative_path() );
    for( point &offsets_offset : offsets ) {
        tripoint_abs_sm submap_addr = project_to<coords::sm>( om_addr );
        submap_addr += offsets_offset;
        submap_addrs.push_back( submap_addr );
        submap *sm = submaps[submap_addr].get();
        if( sm != nullptr ) {
            if( !sm->is_uniform() ) {
                all_uniform = false;
            } else if( sm->reverted ) {
                reverted_to_uniform = file_exists;
            }
        }
    }

    if( all_uniform ) {
        // Nothing to save - this quad will be regenerated faster than it would be re-read
        if( delete_after_save ) {
            for( auto &submap_addr : submap_addrs ) {
                if( submaps.count( submap_addr ) > 0 && submaps[submap_addr] != nullptr ) {
                    submaps_to_delete.push_back( submap_addr );
                }
            }
        }

        // deleting the file might fail on some platforms in some edge cases so force serialize this
        // uniform quad
        if( !reverted_to_uniform ) {
            return;
        }
    }

    // Don't create the directory if it would be empty
    assure_dir_exist( dirname );
    write_to_file( filename, [&]( std::ostream & fout ) {
        JsonOut jsout( fout );
        jsout.start_array();
        for( auto &submap_addr : submap_addrs ) {
            if( submaps.count( submap_addr ) == 0 ) {
                continue;
            }

            submap *sm = submaps[submap_addr].get();

            if( sm == nullptr ) {
                continue;
            }

            jsout.start_object();

            jsout.member( "version", savegame_version );
            jsout.member( "coordinates" );

            jsout.start_array();
            jsout.write( submap_addr.x() );
            jsout.write( submap_addr.y() );
            jsout.write( submap_addr.z() );
            jsout.end_array();

            sm->store( jsout );

            jsout.end_object();

            if( delete_after_save ) {
                submaps_to_delete.push_back( submap_addr );
            }
        }

        jsout.end_array();
    } );

    if( all_uniform && reverted_to_uniform ) {
        fs::remove( filename.get_unrelative_path() );
    }
}

// We're reading in way too many entities here to mess around with creating sub-objects and
// seeking around in them, so we're using the json streaming API.
submap *mapbuffer::unserialize_submaps( const tripoint_abs_sm &p )
{
    // Map the tripoint to the submap quad that stores it.
    const tripoint_abs_omt om_addr = project_to<coords::omt>( p );
    const cata_path dirname = find_dirname( om_addr );
    cata_path quad_path = find_quad_path( dirname, om_addr );

    if( !file_exist( quad_path ) ) {
        // Fix for old saves where the path was generated using std::stringstream, which
        // did format the number using the current locale. That formatting may insert
        // thousands separators, so the resulting path is "map/1,234.7.8.map" instead
        // of "map/1234.7.8.map".
        std::ostringstream buffer;
        buffer << om_addr.x() << "." << om_addr.y() << "." << om_addr.z()
               << ".map";
        cata_path legacy_quad_path = dirname / buffer.str();
        if( file_exist( legacy_quad_path ) ) {
            quad_path = std::move( legacy_quad_path );
        }
    }

    if( !read_from_file_optional_json( quad_path, [this]( const JsonValue & jsin ) {
    deserialize( jsin );
    } ) ) {
        // If it doesn't exist, trigger generating it.
        return nullptr;
    }
    // fill in uniform submaps that were not serialized
    oter_id const oid = overmap_buffer.ter( om_addr );
    generate_uniform_omt( project_to<coords::sm>( om_addr ), oid );
    if( submaps.count( p ) == 0 ) {
        debugmsg( "file %s did not contain the expected submap %s for non-uniform terrain %s",
                  quad_path.generic_u8string(), p.to_string(), oid.id().str() );
        return nullptr;
    }
    return submaps[ p ].get();
}

void mapbuffer::deserialize( const JsonArray &ja )
{
    for( JsonObject submap_json : ja ) {
        std::unique_ptr<submap> sm = std::make_unique<submap>();
        tripoint_abs_sm submap_coordinates;
        int version = 0;
        // We have to read version first because the iteration order of json members is undefined.
        if( submap_json.has_int( "version" ) ) {
            version = submap_json.get_int( "version" );
        }
        for( JsonMember submap_member : submap_json ) {
            std::string submap_member_name = submap_member.name();
            if( submap_member_name == "coordinates" ) {
                JsonArray coords_array = submap_member;
                tripoint_abs_sm loc{ coords_array.next_int(), coords_array.next_int(), coords_array.next_int() };
                submap_coordinates = loc;
            } else {
                sm->load( submap_member, submap_member_name, version );
            }
        }

        if( !add_submap( submap_coordinates, sm ) ) {
            debugmsg( "submap %s was already loaded", submap_coordinates.to_string() );
        }
    }
}
