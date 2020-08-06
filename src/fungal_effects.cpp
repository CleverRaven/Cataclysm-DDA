#include "fungal_effects.h"

#include <algorithm>
#include <memory>
#include <ostream>
#include <string>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "creature.h"
#include "debug.h"
#include "enums.h"
#include "field_type.h"
#include "game.h"
#include "item.h"
#include "item_stack.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "player.h"
#include "point.h"
#include "rng.h"
#include "string_formatter.h"
#include "translations.h"
#include "type_id.h"

static const efftype_id effect_spores( "spores" );
static const efftype_id effect_stunned( "stunned" );

static const skill_id skill_melee( "melee" );

static const mtype_id mon_fungal_blossom( "mon_fungal_blossom" );
static const mtype_id mon_spore( "mon_spore" );

static const species_id species_FUNGUS( "FUNGUS" );

static const trait_id trait_TAIL_CATTLE( "TAIL_CATTLE" );
static const trait_id trait_THRESH_MYCUS( "THRESH_MYCUS" );

static const std::string flag_DIGGABLE( "DIGGABLE" );
static const std::string flag_FLAMMABLE( "FLAMMABLE" );
static const std::string flag_FLAT( "FLAT" );
static const std::string flag_FLOWER( "FLOWER" );
static const std::string flag_FUNGUS( "FUNGUS" );
static const std::string flag_ORGANIC( "ORGANIC" );
static const std::string flag_PLANT( "PLANT" );
static const std::string flag_SHRUB( "SHRUB" );
static const std::string flag_THIN_OBSTACLE( "THIN_OBSTACLE" );
static const std::string flag_TREE( "TREE" );
static const std::string flag_WALL( "WALL" );
static const std::string flag_YOUNG( "YOUNG" );

fungal_effects::fungal_effects( game &g, map &mp )
    : gm( g ), m( mp )
{
}

void fungal_effects::fungalize( const tripoint &p, Creature *origin, double spore_chance )
{
    Character &player_character = get_player_character();
    if( monster *const mon_ptr = g->critter_at<monster>( p ) ) {
        monster &critter = *mon_ptr;
        if( !critter.type->in_species( species_FUNGUS ) ) {
            add_msg_if_player_sees( p, _( "The %s is covered in tiny spores!" ), critter.name() );
        }
        if( !critter.make_fungus() ) {
            // Don't insta-kill non-fungables. Jabberwocks, for example
            critter.add_effect( effect_stunned, rng( 1_turns, 3_turns ) );
            critter.apply_damage( origin, bodypart_id( "torso" ), rng( 25, 50 ) );
        }
    } else if( player_character.pos() == p ) {
        // TODO: Make this accept NPCs when they understand fungals
        ///\EFFECT_DEX increases chance of knocking fungal spores away with your TAIL_CATTLE
        ///\EFFECT_MELEE increases chance of knocking fungal sports away with your TAIL_CATTLE
        if( player_character.has_trait( trait_TAIL_CATTLE ) &&
            one_in( 20 - player_character.dex_cur - player_character.get_skill_level( skill_melee ) ) ) {
            add_msg( _( "The spores land on you, but you quickly swat them off with your tail!" ) );
            return;
        }
        // Spores hit the player--is there any hope?
        bool hit = false;
        hit |= one_in( 4 ) &&
               player_character.add_env_effect( effect_spores, bodypart_id( "head" ), 3, 9_minutes,
                                                bodypart_id( "head" ) );
        hit |= one_in( 2 ) &&
               player_character.add_env_effect( effect_spores, bodypart_id( "torso" ), 3, 9_minutes,
                                                bodypart_id( "torso" ) );
        hit |= one_in( 4 ) &&
               player_character.add_env_effect( effect_spores, bodypart_id( "arm_l" ), 3, 9_minutes,
                                                bodypart_id( "arm_l" ) );
        hit |= one_in( 4 ) &&
               player_character.add_env_effect( effect_spores, bodypart_id( "arm_r" ), 3, 9_minutes,
                                                bodypart_id( "arm_r" ) );
        hit |= one_in( 4 ) &&
               player_character.add_env_effect( effect_spores, bodypart_id( "leg_l" ), 3, 9_minutes,
                                                bodypart_id( "leg_l" ) );
        hit |= one_in( 4 ) &&
               player_character.add_env_effect( effect_spores, bodypart_id( "leg_r" ), 3, 9_minutes,
                                                bodypart_id( "leg_r" ) );
        if( hit ) {
            add_msg( m_warning, _( "You're covered in tiny spores!" ) );
        }
    } else if( gm.num_creatures() < 250 && x_in_y( spore_chance, 1.0 ) ) { // Spawn a spore
        if( monster *const spore = gm.place_critter_at( mon_spore, p ) ) {
            monster *origin_mon = dynamic_cast<monster *>( origin );
            if( origin_mon != nullptr ) {
                spore->make_ally( *origin_mon );
            } else if( origin != nullptr && origin->is_player() &&
                       player_character.has_trait( trait_THRESH_MYCUS ) ) {
                spore->friendly = 1000;
            }
        }
    } else {
        spread_fungus( p );
    }
}

void fungal_effects::create_spores( const tripoint &p, Creature *origin )
{
    for( const tripoint &tmp : get_map().points_in_radius( p, 1 ) ) {
        fungalize( tmp, origin, 0.25 );
    }
}

void fungal_effects::marlossify( const tripoint &p )
{
    const ter_t &terrain = m.ter( p ).obj();
    if( one_in( 25 ) && ( terrain.movecost != 0 && !m.has_furn( p ) )
        && !terrain.has_flag( TFLAG_DEEP_WATER ) ) {
        m.ter_set( p, t_marloss );
        return;
    }
    for( int i = 0; i < 25; i++ ) {
        bool is_fungi = m.has_flag_ter( flag_FUNGUS, p );
        spread_fungus( p );
        if( is_fungi ) {
            return;
        }
    }
}

void fungal_effects::spread_fungus_one_tile( const tripoint &p, const int growth )
{
    bool converted = false;
    // Terrain conversion
    if( m.has_flag_ter( flag_DIGGABLE, p ) ) {
        if( x_in_y( growth * 10, 100 ) ) {
            m.ter_set( p, t_fungus );
            converted = true;
        }
    } else if( m.has_flag( flag_FLAT, p ) ) {
        if( m.has_flag( TFLAG_INDOORS, p ) ) {
            if( x_in_y( growth * 10, 500 ) ) {
                m.ter_set( p, t_fungus_floor_in );
                converted = true;
            }
        } else if( m.has_flag( TFLAG_SUPPORTS_ROOF, p ) ) {
            if( x_in_y( growth * 10, 1000 ) ) {
                m.ter_set( p, t_fungus_floor_sup );
                converted = true;
            }
        } else {
            if( x_in_y( growth * 10, 2500 ) ) {
                m.ter_set( p, t_fungus_floor_out );
                converted = true;
            }
        }
    } else if( m.has_flag( flag_SHRUB, p ) ) {
        if( x_in_y( growth * 10, 200 ) ) {
            m.ter_set( p, t_shrub_fungal );
            converted = true;
        } else if( x_in_y( growth, 1000 ) ) {
            m.ter_set( p, t_marloss );
            converted = true;
        }
    } else if( m.has_flag( flag_THIN_OBSTACLE, p ) ) {
        if( x_in_y( growth * 10, 150 ) ) {
            m.ter_set( p, t_fungus_mound );
            converted = true;
        }
    } else if( m.has_flag( flag_YOUNG, p ) ) {
        if( x_in_y( growth * 10, 500 ) ) {
            if( m.get_field_intensity( p, fd_fungal_haze ) != 0 ) {
                if( x_in_y( growth * 10, 800 ) ) { // young trees are vulnerable
                    m.ter_set( p, t_fungus );
                    if( gm.place_critter_at( mon_fungal_blossom, p ) ) {
                        add_msg_if_player_sees( p, m_warning, _( "The young tree blooms forth into a fungal blossom!" ) );
                    }
                } else if( x_in_y( growth * 10, 400 ) ) {
                    m.ter_set( p, t_marloss_tree );
                }
            } else {
                m.ter_set( p, t_tree_fungal_young );
            }
            converted = true;
        }
    } else if( m.has_flag( flag_TREE, p ) ) {
        if( one_in( 10 ) ) {
            if( m.get_field_intensity( p, fd_fungal_haze ) != 0 ) {
                if( x_in_y( growth * 10, 100 ) ) {
                    m.ter_set( p, t_fungus );
                    if( gm.place_critter_at( mon_fungal_blossom, p ) ) {
                        add_msg_if_player_sees( p, m_warning, _( "The tree blooms forth into a fungal blossom!" ) );
                    }
                } else if( x_in_y( growth * 10, 600 ) ) {
                    m.ter_set( p, t_marloss_tree );
                }
            } else {
                m.ter_set( p, t_tree_fungal );
            }
            converted = true;
        }
    } else if( m.has_flag( flag_WALL, p ) && m.has_flag( flag_FLAMMABLE, p ) ) {
        if( x_in_y( growth * 10, 5000 ) ) {
            m.ter_set( p, t_fungus_wall );
            converted = true;
        }
    }
    // Furniture conversion
    if( converted ) {
        if( m.has_flag( flag_FLOWER, p ) ) {
            m.furn_set( p, f_flower_fungal );
        } else if( m.has_flag( flag_ORGANIC, p ) ) {
            if( m.furn( p ).obj().movecost == -10 ) {
                m.furn_set( p, f_fungal_mass );
            } else {
                m.furn_set( p, f_fungal_clump );
            }
        } else if( m.has_flag( flag_PLANT, p ) ) {
            // Replace the (already existing) seed
            // Can't use item_stack::only_item() since there might be fertilizer
            map_stack items = m.i_at( p );
            const map_stack::iterator seed = std::find_if( items.begin(), items.end(), []( const item & it ) {
                return it.is_seed();
            } );
            if( seed == items.end() || !seed->is_seed() ) {
                DebugLog( D_ERROR, DC_ALL ) << "No seed item in the PLANT terrain at position " <<
                                            string_format( "%d,%d,%d.", p.x, p.y, p.z );
            } else {
                *seed = item( "fungal_seeds", calendar::turn );
            }
        }
    }
}

void fungal_effects::spread_fungus( const tripoint &p )
{
    int growth = 1;
    map &here = get_map();
    for( const tripoint &tmp : here.points_in_radius( p, 1 ) ) {
        if( tmp == p ) {
            continue;
        }
        if( m.has_flag( flag_FUNGUS, tmp ) ) {
            growth += 1;
        }
    }

    if( !m.has_flag_ter( flag_FUNGUS, p ) ) {
        spread_fungus_one_tile( p, growth );
    } else {
        // Everything is already fungus
        if( growth == 9 ) {
            return;
        }
        for( const tripoint &dest : here.points_in_radius( p, 1 ) ) {
            // One spread on average
            if( !m.has_flag( flag_FUNGUS, dest ) && one_in( 9 - growth ) ) {
                //growth chance is 100 in X simplified
                spread_fungus_one_tile( dest, 10 );
            }
        }
    }
}
