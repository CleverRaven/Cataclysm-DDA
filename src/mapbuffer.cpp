#include "mapbuffer.h"

#include <sstream>

#include "cata_utility.h"
#include "computer.h"
#include "coordinate_conversions.h"
#include "debug.h"
#include "filesystem.h"
#include "game.h"
#include "json.h"
#include "map.h"
#include "mapdata.h"
#include "output.h"
#include "submap.h"
#include "translations.h"
#include "trap.h"
#include "vehicle.h"

#define dbg(x) DebugLog((DebugLevel)(x),D_MAP) << __FILE__ << ":" << __LINE__ << ": "

mapbuffer MAPBUFFER;

mapbuffer::mapbuffer() = default;

mapbuffer::~mapbuffer()
{
    reset();
}

void mapbuffer::reset()
{
    for( auto &elem : submaps ) {
        delete elem.second;
    }
    submaps.clear();
}

bool mapbuffer::add_submap( const tripoint &p, submap *sm )
{
    if( submaps.count( p ) != 0 ) {
        return false;
    }

    submaps[p] = sm;

    return true;
}

bool mapbuffer::add_submap( int x, int y, int z, submap *sm )
{
    return add_submap( tripoint( x, y, z ), sm );
}

bool mapbuffer::add_submap( const tripoint &p, std::unique_ptr<submap> &sm )
{
    const bool result = add_submap( p, sm.get() );
    sm.release();
    return result;
}

bool mapbuffer::add_submap( int x, int y, int z, std::unique_ptr<submap> &sm )
{
    return add_submap( tripoint( x, y, z ), sm );
}

void mapbuffer::remove_submap( tripoint addr )
{
    auto m_target = submaps.find( addr );
    if( m_target == submaps.end() ) {
        debugmsg( "Tried to remove non-existing submap %d,%d,%d", addr.x, addr.y, addr.z );
        return;
    }
    delete m_target->second;
    submaps.erase( m_target );
}

submap *mapbuffer::lookup_submap( int x, int y, int z )
{
    return lookup_submap( tripoint( x, y, z ) );
}

submap *mapbuffer::lookup_submap( const tripoint &p )
{
    dbg( D_INFO ) << "mapbuffer::lookup_submap( x[" << p.x << "], y[" << p.y << "], z[" << p.z << "])";

    auto iter = submaps.find( p );
    if( iter == submaps.end() ) {
        try {
            return unserialize_submaps( p );
        } catch( const std::exception &err ) {
            debugmsg( "Failed to load submap (%d,%d,%d): %s", p.x, p.y, p.z, err.what() );
        }
        return nullptr;
    }

    return iter->second;
}

void mapbuffer::save( bool delete_after_save )
{
    std::stringstream map_directory;
    map_directory << g->get_world_base_save_path() << "/maps";
    assure_dir_exist( map_directory.str() );

    int num_saved_submaps = 0;
    int num_total_submaps = submaps.size();

    const tripoint map_origin = sm_to_omt_copy( g->m.get_abs_sub() );
    const bool map_has_zlevels = g != nullptr && g->m.has_zlevels();

    // A set of already-saved submaps, in global overmap coordinates.
    std::set<tripoint> saved_submaps;
    std::list<tripoint> submaps_to_delete;
    int next_report = 0;
    for( auto &elem : submaps ) {
        if( num_total_submaps > 100 && num_saved_submaps >= next_report ) {
            popup_nowait( _( "Please wait as the map saves [%d/%d]" ),
                          num_saved_submaps, num_total_submaps );
            next_report += std::max( 100, num_total_submaps / 20 );
        }

        // Whatever the coordinates of the current submap are,
        // we're saving a 2x2 quad of submaps at a time.
        // Submaps are generated in quads, so we know if we have one member of a quad,
        // we have the rest of it, if that assumption is broken we have REAL problems.
        const tripoint om_addr = sm_to_omt_copy( elem.first );
        if( saved_submaps.count( om_addr ) != 0 ) {
            // Already handled this one.
            continue;
        }
        saved_submaps.insert( om_addr );

        // A segment is a chunk of 32x32 submap quads.
        // We're breaking them into subdirectories so there aren't too many files per directory.
        // Might want to make a set for this one too so it's only checked once per save().
        std::stringstream dirname;
        tripoint segment_addr = omt_to_seg_copy( om_addr );
        dirname << map_directory.str() << "/" << segment_addr.x << "." <<
                segment_addr.y << "." << segment_addr.z;

        std::stringstream quad_path;
        quad_path << dirname.str() << "/" << om_addr.x << "." <<
                  om_addr.y << "." << om_addr.z << ".map";

        // delete_on_save deletes everything, otherwise delete submaps
        // outside the current map.
        const bool zlev_del = !map_has_zlevels && om_addr.z != g->get_levz();
        save_quad( dirname.str(), quad_path.str(), om_addr, submaps_to_delete,
                   delete_after_save || zlev_del ||
                   om_addr.x < map_origin.x || om_addr.y < map_origin.y ||
                   om_addr.x > map_origin.x + HALF_MAPSIZE ||
                   om_addr.y > map_origin.y + HALF_MAPSIZE );
        num_saved_submaps += 4;
    }
    for( auto &elem : submaps_to_delete ) {
        remove_submap( elem );
    }
}

void mapbuffer::save_quad( const std::string &dirname, const std::string &filename,
                           const tripoint &om_addr, std::list<tripoint> &submaps_to_delete,
                           bool delete_after_save )
{
    std::vector<point> offsets;
    std::vector<tripoint> submap_addrs;
    offsets.push_back( point_zero );
    offsets.push_back( point( 0, 1 ) );
    offsets.push_back( point( 1, 0 ) );
    offsets.push_back( point( 1, 1 ) );

    bool all_uniform = true;
    for( auto &offsets_offset : offsets ) {
        tripoint submap_addr = omt_to_sm_copy( om_addr );
        submap_addr.x += offsets_offset.x;
        submap_addr.y += offsets_offset.y;
        submap_addrs.push_back( submap_addr );
        submap *sm = submaps[submap_addr];
        if( sm != nullptr && !sm->is_uniform ) {
            all_uniform = false;
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

        return;
    }

    // Don't create the directory if it would be empty
    assure_dir_exist( dirname );
    ofstream_wrapper_exclusive fout( filename );
    JsonOut jsout( fout );
    jsout.start_array();
    for( auto &submap_addr : submap_addrs ) {
        if( submaps.count( submap_addr ) == 0 ) {
            continue;
        }

        submap *sm = submaps[submap_addr];
        if( sm == nullptr ) {
            continue;
        }

        jsout.start_object();

        jsout.member( "version", savegame_version );

        jsout.member( "coordinates" );
        jsout.start_array();
        jsout.write( submap_addr.x );
        jsout.write( submap_addr.y );
        jsout.write( submap_addr.z );
        jsout.end_array();

        jsout.member( "turn_last_touched", sm->last_touched );
        jsout.member( "temperature", sm->temperature );

        // Terrain is saved using a simple RLE scheme.  Legacy saves don't have
        // this feature but the algorithm is backward compatible.
        jsout.member( "terrain" );
        jsout.start_array();
        std::string last_id;
        int num_same = 1;
        for( int j = 0; j < SEEY; j++ ) {
            for( int i = 0; i < SEEX; i++ ) {
                const std::string this_id = sm->ter[i][j].obj().id.str();
                if( !last_id.empty() ) {
                    if( this_id == last_id ) {
                        num_same++;
                    } else {
                        if( num_same == 1 ) {
                            // if there's only one element don't write as an array
                            jsout.write( last_id );
                        } else {
                            jsout.start_array();
                            jsout.write( last_id );
                            jsout.write( num_same );
                            jsout.end_array();
                            num_same = 1;
                        }
                        last_id = this_id;
                    }
                } else {
                    last_id = this_id;
                }
            }
        }
        // Because of the RLE scheme we have to do one last pass
        if( num_same == 1 ) {
            jsout.write( last_id );
        } else {
            jsout.start_array();
            jsout.write( last_id );
            jsout.write( num_same );
            jsout.end_array();
        }
        jsout.end_array();

        // Write out the radiation array in a simple RLE scheme.
        // written in intensity, count pairs
        jsout.member( "radiation" );
        jsout.start_array();
        int lastrad = -1;
        int count = 0;
        for( int j = 0; j < SEEY; j++ ) {
            for( int i = 0; i < SEEX; i++ ) {
                const point p( i, j );
                // Save radiation, re-examine this because it doesn't look like it works right
                int r = sm->get_radiation( p );
                if( r == lastrad ) {
                    count++;
                } else {
                    if( count ) {
                        jsout.write( count );
                    }
                    jsout.write( r );
                    lastrad = r;
                    count = 1;
                }
            }
        }
        jsout.write( count );
        jsout.end_array();

        jsout.member( "furniture" );
        jsout.start_array();
        for( int j = 0; j < SEEY; j++ ) {
            for( int i = 0; i < SEEX; i++ ) {
                const point p( i, j );
                // Save furniture
                if( sm->get_furn( p ) != f_null ) {
                    jsout.start_array();
                    jsout.write( p.x );
                    jsout.write( p.y );
                    jsout.write( sm->get_furn( p ).obj().id );
                    jsout.end_array();
                }
            }
        }
        jsout.end_array();

        jsout.member( "items" );
        jsout.start_array();
        for( int j = 0; j < SEEY; j++ ) {
            for( int i = 0; i < SEEX; i++ ) {
                if( sm->itm[i][j].empty() ) {
                    continue;
                }
                jsout.write( i );
                jsout.write( j );
                jsout.write( sm->itm[i][j] );
            }
        }
        jsout.end_array();

        jsout.member( "traps" );
        jsout.start_array();
        for( int j = 0; j < SEEY; j++ ) {
            for( int i = 0; i < SEEX; i++ ) {
                const point p( i, j );
                // Save traps
                if( sm->get_trap( p ) != tr_null ) {
                    jsout.start_array();
                    jsout.write( p.x );
                    jsout.write( p.y );
                    // TODO: jsout should support writing an id like jsout.write( trap_id )
                    jsout.write( sm->get_trap( p ).id().str() );
                    jsout.end_array();
                }
            }
        }
        jsout.end_array();

        jsout.member( "fields" );
        jsout.start_array();
        for( int j = 0; j < SEEY; j++ ) {
            for( int i = 0; i < SEEX; i++ ) {
                // Save fields
                if( sm->fld[i][j].fieldCount() > 0 ) {
                    jsout.write( i );
                    jsout.write( j );
                    jsout.start_array();
                    for( auto &fld : sm->fld[i][j] ) {
                        const field_entry &cur = fld.second;
                        // We don't seem to have a string identifier for fields anywhere.
                        jsout.write( cur.getFieldType() );
                        jsout.write( cur.getFieldDensity() );
                        jsout.write( cur.getFieldAge() );
                    }
                    jsout.end_array();
                }
            }
        }
        jsout.end_array();

        // Write out as array of arrays of single entries
        jsout.member( "cosmetics" );
        jsout.start_array();
        for( const auto &cosm : sm->cosmetics ) {
            jsout.start_array();
            jsout.write( cosm.pos.x );
            jsout.write( cosm.pos.y );
            jsout.write( cosm.type );
            jsout.write( cosm.str );
            jsout.end_array();
        }
        jsout.end_array();

        // Output the spawn points
        jsout.member( "spawns" );
        jsout.start_array();
        for( auto &elem : sm->spawns ) {
            jsout.start_array();
            jsout.write( elem.type.str() ); // TODO: json should know how to write string_ids
            jsout.write( elem.count );
            jsout.write( elem.pos.x );
            jsout.write( elem.pos.y );
            jsout.write( elem.faction_id );
            jsout.write( elem.mission_id );
            jsout.write( elem.friendly );
            jsout.write( elem.name );
            jsout.end_array();
        }
        jsout.end_array();

        jsout.member( "vehicles" );
        jsout.start_array();
        for( auto &elem : sm->vehicles ) {
            // json lib doesn't know how to turn a vehicle * into a vehicle,
            // so we have to iterate manually.
            jsout.write( *elem );
        }
        jsout.end_array();

        // Output the computer
        if( sm->comp != nullptr ) {
            jsout.member( "computers", sm->comp->save_data() );
        }

        // Output base camp if any
        if( sm->camp.is_valid() ) {
            jsout.member( "camp", sm->camp );
        }
        if( delete_after_save ) {
            submaps_to_delete.push_back( submap_addr );
        }
        jsout.end_object();
    }

    jsout.end_array();
    fout.close();
}

// We're reading in way too many entities here to mess around with creating sub-objects and
// seeking around in them, so we're using the json streaming API.
submap *mapbuffer::unserialize_submaps( const tripoint &p )
{
    // Map the tripoint to the submap quad that stores it.
    const tripoint om_addr = sm_to_omt_copy( p );
    const tripoint segment_addr = omt_to_seg_copy( om_addr );
    std::stringstream quad_path;
    quad_path << g->get_world_base_save_path() << "/maps/" <<
              segment_addr.x << "." << segment_addr.y << "." << segment_addr.z << "/" <<
              om_addr.x << "." << om_addr.y << "." << om_addr.z << ".map";

    using namespace std::placeholders;
    if( !read_from_file_optional_json( quad_path.str(),
                                       std::bind( &mapbuffer::deserialize, this, _1 ) ) ) {
        // If it doesn't exist, trigger generating it.
        return nullptr;
    }
    if( submaps.count( p ) == 0 ) {
        debugmsg( "file %s did not contain the expected submap %d,%d,%d",
                  quad_path.str().c_str(), p.x, p.y, p.z );
        return nullptr;
    }
    return submaps[ p ];
}

void mapbuffer::deserialize( JsonIn &jsin )
{
    jsin.start_array();
    while( !jsin.end_array() ) {
        std::unique_ptr<submap> sm( new submap() );
        tripoint submap_coordinates;
        jsin.start_object();
        bool rubpow_update = false;
        while( !jsin.end_object() ) {
            std::string submap_member_name = jsin.get_member_name();
            if( submap_member_name == "version" ) {
                if( jsin.get_int() < 22 ) {
                    rubpow_update = true;
                }
            } else if( submap_member_name == "coordinates" ) {
                jsin.start_array();
                int locx = jsin.get_int();
                int locy = jsin.get_int();
                int locz = jsin.get_int();
                jsin.end_array();
                submap_coordinates = tripoint( locx, locy, locz );
            } else if( submap_member_name == "turn_last_touched" ) {
                sm->last_touched = jsin.get_int();
            } else if( submap_member_name == "temperature" ) {
                sm->temperature = jsin.get_int();
            } else if( submap_member_name == "terrain" ) {
                // TODO: try block around this to error out if we come up short?
                jsin.start_array();
                // Small duplication here so that the update check is only performed once
                if( rubpow_update ) {
                    item rock = item( "rock", 0 );
                    item chunk = item( "steel_chunk", 0 );
                    for( int j = 0; j < SEEY; j++ ) {
                        for( int i = 0; i < SEEX; i++ ) {
                            const ter_str_id tid( jsin.get_string() );

                            if( tid == "t_rubble" ) {
                                sm->ter[i][j] = ter_id( "t_dirt" );
                                sm->frn[i][j] = furn_id( "f_rubble" );
                                sm->itm[i][j].push_back( rock );
                                sm->itm[i][j].push_back( rock );
                            } else if( tid == "t_wreckage" ) {
                                sm->ter[i][j] = ter_id( "t_dirt" );
                                sm->frn[i][j] = furn_id( "f_wreckage" );
                                sm->itm[i][j].push_back( chunk );
                                sm->itm[i][j].push_back( chunk );
                            } else if( tid == "t_ash" ) {
                                sm->ter[i][j] = ter_id( "t_dirt" );
                                sm->frn[i][j] = furn_id( "f_ash" );
                            } else if( tid == "t_pwr_sb_support_l" ) {
                                sm->ter[i][j] = ter_id( "t_support_l" );
                            } else if( tid == "t_pwr_sb_switchgear_l" ) {
                                sm->ter[i][j] = ter_id( "t_switchgear_l" );
                            } else if( tid == "t_pwr_sb_switchgear_s" ) {
                                sm->ter[i][j] = ter_id( "t_switchgear_s" );
                            } else {
                                sm->ter[i][j] = tid.id();
                            }
                        }
                    }
                } else {
                    // terrain is encoded using simple RLE
                    int remaining = 0;
                    int_id<ter_t> iid;
                    for( int j = 0; j < SEEY; j++ ) {
                        for( int i = 0; i < SEEX; i++ ) {
                            if( !remaining ) {
                                if( jsin.test_string() ) {
                                    iid = ter_str_id( jsin.get_string() ).id();
                                } else if( jsin.test_array() ) {
                                    jsin.start_array();
                                    iid = ter_str_id( jsin.get_string() ).id();
                                    remaining = jsin.get_int() - 1;
                                    jsin.end_array();
                                } else {
                                    debugmsg( "Mapbuffer terrain data is corrupt, expected string or array." );
                                }
                            } else {
                                --remaining;
                            }
                            sm->ter[i][j] = iid;
                        }
                    }
                    if( remaining ) {
                        debugmsg( "Mapbuffer terrain data is corrupt, tile data remaining." );
                    }
                }
                jsin.end_array();
            } else if( submap_member_name == "radiation" ) {
                int rad_cell = 0;
                jsin.start_array();
                while( !jsin.end_array() ) {
                    int rad_strength = jsin.get_int();
                    int rad_num = jsin.get_int();
                    for( int i = 0; i < rad_num; ++i ) {
                        // A little array trick here, assign to it as a 1D array.
                        // If it's not in bounds we're kinda hosed anyway.
                        sm->set_radiation( { 0, rad_cell }, rad_strength );
                        rad_cell++;
                    }
                }
            } else if( submap_member_name == "furniture" ) {
                jsin.start_array();
                while( !jsin.end_array() ) {
                    jsin.start_array();
                    int i = jsin.get_int();
                    int j = jsin.get_int();
                    sm->frn[i][j] = furn_id( jsin.get_string() );
                    jsin.end_array();
                }
            } else if( submap_member_name == "items" ) {
                jsin.start_array();
                while( !jsin.end_array() ) {
                    int i = jsin.get_int();
                    int j = jsin.get_int();
                    const point p( i, j );
                    jsin.start_array();
                    while( !jsin.end_array() ) {
                        item tmp;
                        jsin.read( tmp );

                        if( tmp.is_emissive() ) {
                            sm->update_lum_add( p, tmp );
                        }

                        tmp.visit_items( [ &sm, &p ]( item * it ) {
                            for( auto &e : it->magazine_convert() ) {
                                sm->itm[p.x][p.y].push_back( e );
                            }
                            return VisitResponse::NEXT;
                        } );

                        sm->itm[p.x][p.y].push_back( tmp );
                        if( tmp.needs_processing() ) {
                            sm->active_items.add( std::prev( sm->itm[p.x][p.y].end() ), p );
                        }
                    }
                }
            } else if( submap_member_name == "traps" ) {
                jsin.start_array();
                while( !jsin.end_array() ) {
                    jsin.start_array();
                    int i = jsin.get_int();
                    int j = jsin.get_int();
                    const point p( i, j );
                    // TODO: jsin should support returning an id like jsin.get_id<trap>()
                    const trap_str_id trid( jsin.get_string() );
                    if( trid == "tr_brazier" ) {
                        sm->frn[p.x][p.y] = furn_id( "f_brazier" );
                    } else {
                        sm->trp[p.x][p.y] = trid.id();
                    }
                    // @todo: remove brazier trap-to-furniture conversion after 0.D
                    jsin.end_array();
                }
            } else if( submap_member_name == "fields" ) {
                jsin.start_array();
                while( !jsin.end_array() ) {
                    // Coordinates loop
                    int i = jsin.get_int();
                    int j = jsin.get_int();
                    jsin.start_array();
                    while( !jsin.end_array() ) {
                        int type = jsin.get_int();
                        int density = jsin.get_int();
                        int age = jsin.get_int();
                        if( sm->fld[i][j].findField( field_id( type ) ) == nullptr ) {
                            sm->field_count++;
                        }
                        sm->fld[i][j].addField( field_id( type ), density, time_duration::from_turns( age ) );
                    }
                }
            } else if( submap_member_name == "graffiti" ) {
                jsin.start_array();
                while( !jsin.end_array() ) {
                    jsin.start_array();
                    int i = jsin.get_int();
                    int j = jsin.get_int();
                    const point p( i, j );
                    sm->set_graffiti( p, jsin.get_string() );
                    jsin.end_array();
                }
            } else if( submap_member_name == "cosmetics" ) {
                jsin.start_array();
                std::map<std::string, std::string> tcosmetics;

                while( !jsin.end_array() ) {
                    jsin.start_array();
                    int i = jsin.get_int();
                    int j = jsin.get_int();
                    const point p( i, j );
                    std::string type, str;
                    // Try to read as current format
                    if( jsin.test_string() ) {
                        type = jsin.get_string();
                        str = jsin.get_string();
                        sm->insert_cosmetic( p, type, str );
                    } else {
                        // Otherwise read as most recent old format
                        jsin.read( tcosmetics );
                        for( auto &cosm : tcosmetics ) {
                            sm->insert_cosmetic( p, cosm.first, cosm.second );
                        }
                        tcosmetics.clear();
                    }

                    jsin.end_array();
                }
            } else if( submap_member_name == "spawns" ) {
                jsin.start_array();
                while( !jsin.end_array() ) {
                    jsin.start_array();
                    // TODO: json should know how to read an string_id
                    const mtype_id type = mtype_id( jsin.get_string() );
                    int count = jsin.get_int();
                    int i = jsin.get_int();
                    int j = jsin.get_int();
                    const point p( i, j );
                    int faction_id = jsin.get_int();
                    int mission_id = jsin.get_int();
                    bool friendly = jsin.get_bool();
                    std::string name = jsin.get_string();
                    jsin.end_array();
                    spawn_point tmp( type, count, p, faction_id, mission_id, friendly, name );
                    sm->spawns.push_back( tmp );
                }
            } else if( submap_member_name == "vehicles" ) {
                jsin.start_array();
                while( !jsin.end_array() ) {
                    std::unique_ptr<vehicle> tmp( new vehicle() );
                    jsin.read( *tmp );
                    sm->vehicles.push_back( std::move( tmp ) );
                }
            } else if( submap_member_name == "computers" ) {
                std::string computer_data = jsin.get_string();
                std::unique_ptr<computer> new_comp( new computer( "BUGGED_COMPUTER", -100 ) );
                new_comp->load_data( computer_data );
                sm->comp = std::move( new_comp );
            } else if( submap_member_name == "camp" ) {
                jsin.read( sm->camp );
            } else {
                jsin.skip_value();
            }
        }
        if( !add_submap( submap_coordinates, sm ) ) {
            debugmsg( "submap %d,%d,%d was already loaded", submap_coordinates.x, submap_coordinates.y,
                      submap_coordinates.z );
        }
    }
}
