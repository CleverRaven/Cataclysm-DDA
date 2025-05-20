#include <algorithm>

#include "coordinates.h"
#include "scent_block.h"
#include "scent_map.h"

scent_block::scent_block( const tripoint_bub_sm &sub, scent_map &scents )
// NOLINTNEXTLINE(cata-use-named-point-constants)
    : origin( coords::project_to<coords::ms>( sub ) + point( -1, -1 ) )
    , scents( scents )
    , modification_count( 0 )
{
    for( int x = 0; x < SEEX + 2; ++x ) {
        for( int y = 0; y < SEEY + 2; ++y ) {
            assignment[x][y] = { data_mode::NONE, 0 };
        }
    }
}

void scent_block::commit_modifications()
{
    if( modification_count == 0 ) {
        return;
    }
    for( int x = 0; x < SEEX + 2; ++x ) {
        for( int y = 0; y < SEEY + 2; ++y ) {
            switch( assignment[x][y].mode ) {
                case data_mode::NONE:
                    break;
                case data_mode::SET: {
                    tripoint_bub_ms p{ origin + tripoint( x, y, 0 ) };
                    if( scents.inbounds( p ) ) {
                        scents.set_unsafe( p, assignment[x][y].intensity );
                    }
                    break;
                }
                case data_mode::MAX: {
                    tripoint_bub_ms p{ origin + tripoint( x, y, 0 ) };
                    if( scents.inbounds( p ) ) {
                        scents.set_unsafe( p, std::max( assignment[x][y].intensity, scents.get_unsafe( p ) ) );
                    }
                    break;
                }
            }
        }
    }
}

// We should be working entirely within the range, so don't range check here
void scent_block::apply_gas( const tripoint_bub_ms &p, const int nintensity )
{
    const point_rel_ms ndx = index( p );
    assignment[ndx.x()][ndx.y()].mode = data_mode::SET;
    assignment[ndx.x()][ndx.y()].intensity = std::max( 0,
            assignment[ndx.x()][ndx.y()].intensity - nintensity );
    ++modification_count;
}

void scent_block::apply_slime( const tripoint_bub_ms &p, int intensity )
{
    const point_rel_ms ndx = index( p );
    datum &dat = assignment[ndx.x()][ndx.y()];
    switch( dat.mode ) {
        case data_mode::NONE: {
            // we don't know what the current intensity is, so we must do a max operation
            dat.mode = data_mode::MAX;
            dat.intensity = intensity;
            break;
        }
        case data_mode::MAX:
        // Already max for some reason, shouldn't occur.
        // If it does we want to grow if possible
        case data_mode::SET: {
            // new intensity is going to be dat.intensity, so we just need to make it larger
            // but cannot change
            dat.intensity = std::max( dat.intensity, intensity );
            break;
        }
    }
    ++modification_count;
}
