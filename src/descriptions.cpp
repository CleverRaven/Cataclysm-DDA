#include "game.h" // IWYU pragma: associated

#include <algorithm>
#include <map>
#include <sstream>
#include <string>
#include <utility>

#include "avatar.h"
#include "calendar.h"
#include "character.h"
#include "color.h"
#include "creature_tracker.h"
#include "harvest.h"
#include "input_context.h"
#include "map.h"
#include "mapdata.h"
#include "mod_manager.h"
#include "output.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui_manager.h"
#include "viewer.h"

static const skill_id skill_survival( "survival" );

static const trait_id trait_ILLITERATE( "ILLITERATE" );

enum class description_target : int {
    creature,
    furniture,
    terrain,
    vehicle
};

static const Creature *seen_critter( const tripoint_bub_ms &p )
{
    const Creature *critter = get_creature_tracker().creature_at( p, true );
    if( critter != nullptr && get_player_view().sees( *critter ) ) {
        return critter;
    }

    return nullptr;
}

void game::extended_description( const tripoint_bub_ms &p )
{
    ui_adaptor ui;
    const int top = 3;
    int width = 0;
    catacurses::window w_head;
    catacurses::window w_main;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        const int left = 0;
        const int right = TERMX;
        const int bottom = TERMY;
        width = right - left;
        const int height = bottom - top;
        w_head = catacurses::newwin( top, TERMX, point::zero );
        w_main = catacurses::newwin( height, width, point( left, top ) );
        ui.position( point::zero, point( TERMX, TERMY ) );
    } );
    ui.mark_resize();

    // Default to critter (if any), furniture (if any), then terrain.
    description_target cur_target = description_target::terrain;
    if( seen_critter( p ) != nullptr ) {
        cur_target = description_target::creature;
    } else if( get_map().has_furn( p ) ) {
        cur_target = description_target::furniture;
    }

    std::string action;
    input_context ctxt( "EXTENDED_DESCRIPTION" );
    ctxt.register_action( "CREATURE" );
    ctxt.register_action( "FURNITURE" );
    ctxt.register_action( "TERRAIN" );
    ctxt.register_action( "VEHICLE" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( w_head );
        mvwprintz( w_head, point::zero, c_white,
                   _( "[%s] describe creatures, [%s] describe furniture, "
                      "[%s] describe terrain, [%s] describe vehicle/appliance, [%s] close." ),
                   ctxt.get_desc( "CREATURE" ), ctxt.get_desc( "FURNITURE" ),
                   ctxt.get_desc( "TERRAIN" ), ctxt.get_desc( "VEHICLE" ), ctxt.get_desc( "QUIT" ) );

        // Set up line drawings
        for( int i = 0; i < TERMX; i++ ) {
            mvwputch( w_head, point( i, top - 1 ), c_white, LINE_OXOX );
        }

        wnoutrefresh( w_head );

        std::string desc;
        // Allow looking at invisible tiles - player may want to examine hallucinations etc.
        switch( cur_target ) {
            case description_target::creature: {
                const Creature *critter = seen_critter( p );
                if( critter != nullptr ) {
                    desc = critter->extended_description().front();
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
                    const std::string mod_src = enumerate_as_string( fid->src.begin(),
                    fid->src.end(), []( const std::pair<furn_str_id, mod_id> &source ) {
                        return string_format( "'%s'", source.second->name() );
                    }, enumeration_conjunction::arrow );
                    desc = string_format( _( "Origin: %s\n%s" ), mod_src, fid->extended_description().front() );
                }
                break;
            case description_target::terrain:
                if( !u.sees( p ) ) {
                    desc = _( "You can't see the terrain here." );
                } else {
                    const ter_id tid = m.ter( p );
                    const std::string mod_src = enumerate_as_string( tid->src.begin(),
                    tid->src.end(), []( const std::pair<ter_str_id, mod_id> &source ) {
                        return string_format( "'%s'", source.second->name() );
                    }, enumeration_conjunction::arrow );
                    desc = string_format( _( "Origin: %s\n%s" ), mod_src, tid->extended_description().front() );
                }
                break;
            case description_target::vehicle:
                const optional_vpart_position vp = m.veh_at( p );
                if( !u.sees( p ) || !vp ) {
                    desc = _( "You can't see vehicles or appliances here." );
                } else {
                    desc = vp.extended_description().front();
                }
                break;
        }

        std::string signage = m.get_signage( p );
        if( !signage.empty() ) {
            // NOLINTNEXTLINE(cata-text-style): the question mark does not end a sentence
            desc += u.has_trait( trait_ILLITERATE ) ? _( "\nSign: ???" ) : string_format( _( "\nSign: %s" ),
                    signage );
        }

        werase( w_main );
        fold_and_print_from( w_main, point::zero, width, 0, c_light_gray, desc );
        wnoutrefresh( w_main );
    } );

    do {
        ui_manager::redraw();
        action = ctxt.handle_input();
        if( action == "CREATURE" ) {
            cur_target = description_target::creature;
        } else if( action == "FURNITURE" ) {
            cur_target = description_target::furniture;
        } else if( action == "TERRAIN" ) {
            cur_target = description_target::terrain;
        } else if( action == "VEHICLE" ) {
            cur_target = description_target::vehicle;
        }
    } while( action != "CONFIRM" && action != "QUIT" );
}
