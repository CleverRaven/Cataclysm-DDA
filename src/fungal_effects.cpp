#include "fungal_effects.h"

#include "creature.h"
#include "field.h"
#include "game.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "output.h"
#include "player.h"

const mtype_id mon_fungal_blossom( "mon_fungal_blossom" );
const mtype_id mon_spore( "mon_spore" );

const efftype_id effect_stunned( "stunned" );
const efftype_id effect_spores( "spores" );

const species_id FUNGUS( "FUNGUS" );

fungal_effects::fungal_effects( game &g, map &mp )
    : gm( g ), m( mp )
{
}

void fungal_effects::fungalize( const tripoint &p, Creature *origin, double spore_chance )
{
    if( monster *const mon_ptr = g->critter_at<monster>( p ) ) {
        monster &critter = *mon_ptr;
        if( gm.u.sees( p ) &&
            !critter.type->in_species( FUNGUS ) ) {
            add_msg( _( "The %s is covered in tiny spores!" ),
                     critter.name().c_str() );
        }
        if( !critter.make_fungus() ) {
            // Don't insta-kill non-fungables. Jabberwocks, for example
            critter.add_effect( effect_stunned, rng( 1_turns, 3_turns ) );
            critter.apply_damage( origin, bp_torso, rng( 25, 50 ) );
        }
    } else if( gm.u.pos() == p ) {
        player &pl = gm.u; // TODO: Make this accept NPCs when they understand fungals
        ///\EFFECT_DEX increases chance of knocking fungal spores away with your TAIL_CATTLE

        ///\EFFECT_MELEE increases chance of knocking fungal sports away with your TAIL_CATTLE
        if( pl.has_trait( trait_id( "TAIL_CATTLE" ) ) &&
            one_in( 20 - pl.dex_cur - pl.get_skill_level( skill_id( "melee" ) ) ) ) {
            pl.add_msg_if_player(
                _( "The spores land on you, but you quickly swat them off with your tail!" ) );
            return;
        }
        // Spores hit the player--is there any hope?
        bool hit = false;
        hit |= one_in( 4 ) && pl.add_env_effect( effect_spores, bp_head, 3, 9_minutes, bp_head );
        hit |= one_in( 2 ) && pl.add_env_effect( effect_spores, bp_torso, 3, 9_minutes, bp_torso );
        hit |= one_in( 4 ) && pl.add_env_effect( effect_spores, bp_arm_l, 3, 9_minutes, bp_arm_l );
        hit |= one_in( 4 ) && pl.add_env_effect( effect_spores, bp_arm_r, 3, 9_minutes, bp_arm_r );
        hit |= one_in( 4 ) && pl.add_env_effect( effect_spores, bp_leg_l, 3, 9_minutes, bp_leg_l );
        hit |= one_in( 4 ) && pl.add_env_effect( effect_spores, bp_leg_r, 3, 9_minutes, bp_leg_r );
        if( hit ) {
            add_msg( m_warning, _( "You're covered in tiny spores!" ) );
        }
    } else if( gm.num_creatures() < 250 && x_in_y( spore_chance, 1.0 ) ) { // Spawn a spore
        if( monster *const spore = gm.summon_mon( mon_spore, p ) ) {
            monster *origin_mon = dynamic_cast<monster *>( origin );
            if( origin_mon != nullptr ) {
                spore->make_ally( *origin_mon );
            } else if( origin != nullptr && origin->is_player() &&
                       gm.u.has_trait( trait_id( "THRESH_MYCUS" ) ) ) {
                spore->friendly = 1000;
            }
        }
    } else {
        spread_fungus( p );
    }
}

void fungal_effects::create_spores( const tripoint &p, Creature *origin )
{
    for( const tripoint &tmp : g->m.points_in_radius( p, 1 ) ) {
        fungalize( tmp, origin, 0.25 );
    }
}

bool fungal_effects::marlossify( const tripoint &p )
{
    auto &terrain = m.ter( p ).obj();
    if( one_in( 25 ) && ( terrain.movecost != 0 && !m.has_furn( p ) )
        && !terrain.has_flag( TFLAG_DEEP_WATER ) ) {
        m.ter_set( p, t_marloss );
        return true;
    }
    for( int i = 0; i < 25; i++ ) {
        if( !spread_fungus( p ) ) {
            return true;
        }
    }
    return false;
}

bool fungal_effects::spread_fungus( const tripoint &p )
{
    int growth = 1;
    for( const tripoint &tmp : g->m.points_in_radius( p, 1 ) ) {
        if( tmp == p ) {
            continue;
        }
        if( m.has_flag( "FUNGUS", tmp ) ) {
            growth += 1;
        }
    }

    bool converted = false;
    if( !m.has_flag_ter( "FUNGUS", p ) ) {
        // Terrain conversion
        if( m.has_flag_ter( "DIGGABLE", p ) ) {
            if( x_in_y( growth * 10, 100 ) ) {
                m.ter_set( p, t_fungus );
                converted = true;
            }
        } else if( m.has_flag( "FLAT", p ) ) {
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
        } else if( m.has_flag( "SHRUB", p ) ) {
            if( x_in_y( growth * 10, 200 ) ) {
                m.ter_set( p, t_shrub_fungal );
                converted = true;
            } else if( x_in_y( growth, 1000 ) ) {
                m.ter_set( p, t_marloss );
                converted = true;
            }
        } else if( m.has_flag( "THIN_OBSTACLE", p ) ) {
            if( x_in_y( growth * 10, 150 ) ) {
                m.ter_set( p, t_fungus_mound );
                converted = true;
            }
        } else if( m.has_flag( "YOUNG", p ) ) {
            if( x_in_y( growth * 10, 500 ) ) {
                m.ter_set( p, t_tree_fungal_young );
                converted = true;
            }
        } else if( m.has_flag( "WALL", p ) ) {
            if( x_in_y( growth * 10, 5000 ) ) {
                converted = true;
                m.ter_set( p, t_fungus_wall );
            }
        }
        // Furniture conversion
        if( converted ) {
            if( m.has_flag( "FLOWER", p ) ) {
                m.furn_set( p, f_flower_fungal );
            } else if( m.has_flag( "ORGANIC", p ) ) {
                if( m.furn( p ).obj().movecost == -10 ) {
                    m.furn_set( p, f_fungal_mass );
                } else {
                    m.furn_set( p, f_fungal_clump );
                }
            } else if( m.has_flag( "PLANT", p ) ) {
                // Replace the (already existing) seed
                m.i_at( p )[0] = item( "fungal_seeds", calendar::turn );
            }
        }
        return true;
    } else {
        // Everything is already fungus
        if( growth == 9 ) {
            return false;
        }
        for( const tripoint &dest : g->m.points_in_radius( p, 1 ) ) {
            // One spread on average
            if( !m.has_flag( "FUNGUS", dest ) && one_in( 9 - growth ) ) {
                //growth chance is 100 in X simplified
                if( m.has_flag( "DIGGABLE", dest ) ) {
                    m.ter_set( dest, t_fungus );
                    converted = true;
                } else if( m.has_flag( "FLAT", dest ) ) {
                    if( m.has_flag( TFLAG_INDOORS, dest ) ) {
                        if( one_in( 5 ) ) {
                            m.ter_set( dest, t_fungus_floor_in );
                            converted = true;
                        }
                    } else if( m.has_flag( TFLAG_SUPPORTS_ROOF, dest ) ) {
                        if( one_in( 10 ) ) {
                            m.ter_set( dest, t_fungus_floor_sup );
                            converted = true;
                        }
                    } else {
                        if( one_in( 25 ) ) {
                            m.ter_set( dest, t_fungus_floor_out );
                            converted = true;
                        }
                    }
                } else if( m.has_flag( "SHRUB", dest ) ) {
                    if( one_in( 2 ) ) {
                        m.ter_set( dest, t_shrub_fungal );
                        converted = true;
                    } else if( one_in( 25 ) ) {
                        m.ter_set( dest, t_marloss );
                        converted = true;
                    }
                } else if( m.has_flag( "THIN_OBSTACLE", dest ) ) {
                    if( x_in_y( 10, 15 ) ) {
                        m.ter_set( dest, t_fungus_mound );
                        converted = true;
                    }
                } else if( m.has_flag( "YOUNG", dest ) ) {
                    if( one_in( 5 ) ) {
                        if( m.get_field_strength( p, fd_fungal_haze ) != 0 ) {
                            if( one_in( 8 ) ) { // young trees are vulnerable
                                m.ter_set( dest, t_fungus );
                                gm.summon_mon( mon_fungal_blossom, p );
                                if( gm.u.sees( p ) ) {
                                    add_msg( m_warning, _( "The young tree blooms forth into a fungal blossom!" ) );
                                }
                            } else if( one_in( 4 ) ) {
                                m.ter_set( dest, t_marloss_tree );
                            }
                        } else {
                            m.ter_set( dest, t_tree_fungal_young );
                        }
                        converted = true;
                    }
                } else if( m.has_flag( "TREE", dest ) ) {
                    if( one_in( 10 ) ) {
                        if( m.get_field_strength( p, fd_fungal_haze ) != 0 ) {
                            if( one_in( 10 ) ) {
                                m.ter_set( dest, t_fungus );
                                gm.summon_mon( mon_fungal_blossom, p );
                                if( gm.u.sees( p ) ) {
                                    add_msg( m_warning, _( "The tree blooms forth into a fungal blossom!" ) );
                                }
                            } else if( one_in( 6 ) ) {
                                m.ter_set( dest, t_marloss_tree );
                            }
                        } else {
                            m.ter_set( dest, t_tree_fungal );
                        }
                        converted = true;
                    }
                } else if( m.has_flag( "WALL", dest ) ) {
                    if( one_in( 50 ) ) {
                        converted = true;
                        m.ter_set( dest, t_fungus_wall );
                    }
                }

                if( converted ) {
                    if( m.has_flag( "FLOWER", dest ) ) {
                        m.furn_set( dest, f_flower_fungal );
                    } else if( m.has_flag( "ORGANIC", dest ) ) {
                        if( m.furn( dest ).obj().movecost == -10 ) {
                            m.furn_set( dest, f_fungal_mass );
                        } else {
                            m.furn_set( dest, f_fungal_clump );
                        }
                    } else if( m.has_flag( "PLANT", dest ) ) {
                        // Replace the (already existing) seed
                        m.i_at( p )[0] = item( "fungal_seeds", calendar::turn );
                    }
                }
            }
        }
        return false;
    }
}
