#include "game.h" // IWYU pragma: associated

#include <algorithm>
#include <sstream>
#include <utility>

#include "avatar.h"
#include "calendar.h"
#include "harvest.h"
#include "input.h"
#include "map.h"
#include "mapdata.h"
#include "output.h"
#include "string_formatter.h"
#include "color.h"
#include "translations.h"
#include "string_id.h"

const skill_id skill_survival( "survival" );

static const trait_id trait_ILLITERATE( "ILLITERATE" );

enum class description_target : int {
    creature,
    furniture,
    terrain
};

static const Creature *seen_critter( const game &g, const tripoint &p )
{
    const Creature *critter = g.critter_at( p, true );
    if( critter != nullptr && g.u.sees( *critter ) ) {
        return critter;
    }

    return nullptr;
}

void game::extended_description( const tripoint &p )
{
    const int left = 0;
    const int right = TERMX;
    const int top = 3;
    const int bottom = TERMY;
    const int width = right - left;
    const int height = bottom - top;
    catacurses::window w_head = catacurses::newwin( top, TERMX, point_zero );
    catacurses::window w_main = catacurses::newwin( height, width, point( left, top ) );
    // TODO: De-hardcode
    std::string header_message = _( "\
c to describe creatures, f to describe furniture, t to describe terrain, Esc/Enter to close." );
    mvwprintz( w_head, point_zero, c_white, header_message );

    // Set up line drawings
    for( int i = 0; i < TERMX; i++ ) {
        mvwputch( w_head, point( i, top - 1 ), c_white, LINE_OXOX );
    }

    wrefresh( w_head );
    // Default to critter (if any), furniture (if any), then terrain.
    description_target cur_target = description_target::terrain;
    if( seen_critter( *this, p ) != nullptr ) {
        cur_target = description_target::creature;
    } else if( g->m.has_furn( p ) ) {
        cur_target = description_target::furniture;
    }
    int ch = 'c';
    do {
        std::string desc;
        // Allow looking at invisible tiles - player may want to examine hallucinations etc.
        switch( cur_target ) {
            case description_target::creature: {
                const Creature *critter = seen_critter( *this, p );
                if( critter != nullptr ) {
                    desc = critter->extended_description();
                } else {
                    desc = _( "You do not see any creature here." );
                }
            }
            break;
            case description_target::furniture:
                if( !u.sees( p ) || !m.has_furn( p ) ) {
                    desc = _( "You do not see any furniture here." );
                } else {
                    const furn_id fid = m.furn( p );
                    desc = fid.obj().extended_description();
                }
                break;
            case description_target::terrain:
                if( !u.sees( p ) ) {
                    desc = _( "You can't see the terrain here." );
                } else {
                    const ter_id tid = m.ter( p );
                    desc = tid.obj().extended_description();
                }
                break;
        }

        std::string signage = m.get_signage( p );
        if( !signage.empty() ) {
            desc += u.has_trait( trait_ILLITERATE ) ? string_format( _( "\nSign: ???" ) ) : string_format(
                        _( "\nSign: %s" ), signage );
        }

        werase( w_main );
        fold_and_print_from( w_main, point_zero, width, 0, c_light_gray, desc );
        wrefresh( w_main );
        // TODO: use input context
        ch = inp_mngr.get_input_event().get_first_input();
        switch( ch ) {
            case 'c':
                cur_target = description_target::creature;
                break;
            case 'f':
                cur_target = description_target::furniture;
                break;
            case 't':
                cur_target = description_target::terrain;
                break;
        }

    } while( ch != KEY_ESCAPE && ch != '\n' );
}

std::string map_data_common_t::extended_description() const
{
    std::stringstream ss;
    ss << "<header>" << string_format( _( "That is a %s." ), name() ) << "</header>" << '\n';
    ss << description.translated() << std::endl;
    bool has_any_harvest = std::any_of( harvest_by_season.begin(), harvest_by_season.end(),
    []( const harvest_id & hv ) {
        return !hv.obj().empty();
    } );

    if( has_any_harvest ) {
        ss << "--" << std::endl;
        int player_skill = g->u.get_skill_level( skill_survival );
        ss << _( "You could harvest the following things from it:" ) << std::endl;
        // Group them by identical ids to avoid repeating same blocks of data
        // First, invert the mapping: season->id to id->seasons
        std::multimap<harvest_id, season_type> identical_harvest;
        for( size_t season = SPRING; season <= WINTER; season++ ) {
            const auto &hv = harvest_by_season[ season ];
            if( hv.obj().empty() ) {
                continue;
            }

            identical_harvest.insert( std::make_pair( hv, static_cast<season_type>( season ) ) );
        }
        // Now print them in order of seasons
        // TODO: Highlight current season
        for( size_t season = SPRING; season <= WINTER; season++ ) {
            const auto range = identical_harvest.equal_range( harvest_by_season[ season ] );
            if( range.first == range.second ) {
                continue;
            }

            // List the seasons first
            ss << enumerate_as_string( range.first, range.second,
            []( const std::pair<harvest_id, season_type> &pr ) {
                if( pr.second == season_of_year( calendar::turn ) ) {
                    return "<good>" + calendar::name_season( pr.second ) + "</good>";
                }

                return "<dark>" + calendar::name_season( pr.second ) + "</dark>";
            } );
            ss << ":" << std::endl;
            // List the drops
            // They actually describe what player can get from it now, so it isn't spoily
            // TODO: Allow spoily listing of everything
            ss << range.first->first.obj().describe( player_skill ) << std::endl;
            // Remove the range from the multimap so that it isn't listed twice
            identical_harvest.erase( range.first, range.second );
        }

        ss << std::endl;
    }

    return replace_colors( ss.str() );
}
