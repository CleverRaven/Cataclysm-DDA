#include "activity_actor_definitions.h"
#include "character.h"
#include "flag.h"
#include "item.h"
#include "itype.h"
#include "map_selector.h"
#include "vehicle_selector.h"

static const trait_id trait_DEBUG_HS( "DEBUG_HS" );
static const itype_id itype_small_repairkit( "small_repairkit" );
static const itype_id itype_large_repairkit( "large_repairkit" );

template <typename T, typename Output>
void find_ammo_helper( T &src, const item &obj, bool empty, Output out, bool nested )
{
    if( obj.is_watertight_container() ) {
        if( !obj.is_container_empty() ) {

            // Look for containers with the same type of liquid as that already in our container
            src.visit_items( [&src, &nested, &out, &obj]( item * node, item * parent ) {
                if( node == &obj ) {
                    // This stops containers and magazines counting *themselves* as ammo sources
                    return VisitResponse::SKIP;
                }
                // Prevents reloading with items frozen in watertight containers.
                if( parent != nullptr && parent->is_watertight_container() && node->is_frozen_liquid() ) {
                    return VisitResponse::SKIP;
                }

                // Liquids not in a watertight container are skipped.
                if( parent != nullptr && !parent->is_watertight_container() &&
                    node->made_of( phase_id::LIQUID ) ) {
                    return VisitResponse::SKIP;
                }

                // Spills have no parent.
                if( parent == nullptr && node->made_of_from_type( phase_id::LIQUID ) ) {
                    return VisitResponse::SKIP;
                }

                if( !nested && node->is_container() && parent != nullptr && parent->is_container() ) {
                    return VisitResponse::SKIP;
                }

                if( node->made_of_from_type( phase_id::LIQUID ) ) {
                    out = item_location( item_location( src, parent ), node );
                }

                return VisitResponse::NEXT;
            } );
        } else {
            // Look for containers with any liquid and loose frozen liquids
            src.visit_items( [&src, &nested, &out]( item * node, item * parent ) {
                // Prevents reloading with items frozen in watertight containers.
                if( parent != nullptr && parent->is_watertight_container() && node->is_frozen_liquid() ) {
                    return VisitResponse::SKIP;
                }

                // Liquids not in a watertight container are skipped.
                if( parent != nullptr && !parent->is_watertight_container() &&
                    node->made_of( phase_id::LIQUID ) ) {
                    return VisitResponse::SKIP;
                }

                // Spills have no parent.
                if( parent == nullptr && node->made_of_from_type( phase_id::LIQUID ) ) {
                    return VisitResponse::SKIP;
                }

                if( !nested && node->is_container() && parent != nullptr && parent->is_container() ) {
                    return VisitResponse::SKIP;
                }

                if( node->made_of_from_type( phase_id::LIQUID ) ) {
                    out = item_location( item_location( src, parent ), node );
                }

                return VisitResponse::NEXT;
            } );
        }
    }
    if( obj.magazine_integral() ) {
        // find suitable ammo excluding that already loaded in magazines
        std::set<ammotype> ammo = obj.ammo_types();

        src.visit_items( [&src, &nested, &out, ammo]( item * node, item * parent ) {
            if( !node->made_of_from_type( phase_id::SOLID ) && parent == nullptr ) {
                // some liquids are ammo but we can't reload with them unless within a container or frozen
                return VisitResponse::SKIP;
            }
            if( !node->made_of( phase_id::SOLID ) && parent != nullptr ) {
                for( const ammotype &at : ammo ) {
                    if( node->ammo_type() == at ) {
                        out = item_location( src, node );
                    }
                }
                return VisitResponse::SKIP;
            }

            // Solid ammo gets skipped earlier than non-solid because it does not need a container.
            if( !nested && parent != nullptr && parent->is_container() &&
                !node->made_of_from_type( phase_id::LIQUID ) && !node->made_of( phase_id::GAS ) ) {
                return VisitResponse::SKIP;
            }

            if( !nested && node->is_container() && parent != nullptr && parent->is_container() ) {
                return VisitResponse::SKIP;
            }

            // ammo is inside some sort of a container
            if( parent != nullptr && parent->is_container() ) {
                for( const ammotype &at : ammo ) {
                    if( node->ammo_type() == at ) {
                        out = item_location( item_location( src, parent ), node );
                    }
                }
                if( node->is_magazine() &&
                    ( parent == nullptr || node != parent->magazine_current() ) &&
                    node->has_flag( flag_SPEEDLOADER ) ) {
                    if( node->ammo_remaining() ) {
                        out = item_location( item_location( src, parent ), node );
                    }
                }
                return VisitResponse::NEXT;
            }

            // everything else, probably?
            for( const ammotype &at : ammo ) {
                if( node->ammo_type() == at ) {
                    out = item_location( src, node );
                }
            }
            if( node->is_magazine() &&
                ( parent == nullptr || node != parent->magazine_current() ) &&
                node->has_flag( flag_SPEEDLOADER ) ) {
                if( node->ammo_remaining() ) {
                    out = item_location( src, node );
                }
            }
            return VisitResponse::NEXT;
        } );
    } else {
        // find compatible magazines excluding those already loaded in tools/guns
        src.visit_items( [&src, &nested, &out, &obj, empty]( item * node, item * parent ) {
            // magazine is inside some sort of a container
            if( parent != nullptr && node != parent->magazine_current() && parent->is_container() ) {
                if( obj.can_contain( *node, true ).success() && ( node->ammo_remaining() || empty ) ) {
                    out = item_location( item_location( src, parent ), node );
                }
                return node->is_magazine() ? VisitResponse::SKIP : VisitResponse::NEXT;
            }
            //everything else, probably?
            if( parent == nullptr || node != parent->magazine_current() ) {
                if( obj.can_contain( *node, true ).success() && ( node->ammo_remaining() || empty ) ) {
                    out = item_location( src, node );
                }
                return node->is_magazine() ? VisitResponse::SKIP : VisitResponse::NEXT;
            }
            return nested ? VisitResponse::NEXT : VisitResponse::SKIP;
        } );
    }
}

std::vector<const item *> Character::get_ammo( const ammotype &at ) const
{
    return items_with( [at]( const item & it ) {
        return it.ammo_type() == at;
    } );
}

std::vector<item_location> Character::find_ammo( const item &obj, bool empty, int radius ) const
{
    std::vector<item_location> res;

    find_ammo_helper( const_cast<Character &>( *this ), obj, empty, std::back_inserter( res ), true );

    if( radius >= 0 ) {
        for( auto &cursor : map_selector( pos(), radius ) ) {
            find_ammo_helper( cursor, obj, empty, std::back_inserter( res ), false );
        }
        for( auto &cursor : vehicle_selector( pos(), radius ) ) {
            find_ammo_helper( cursor, obj, empty, std::back_inserter( res ), false );
        }
    }

    return res;
}

std::pair<int, int> Character::gunmod_installation_odds( const item &gun, const item &mod ) const
{
    // Mods with INSTALL_DIFFICULT have a chance to fail, potentially damaging the gun
    if( !mod.has_flag( flag_INSTALL_DIFFICULT ) || has_trait( trait_DEBUG_HS ) ) {
        return std::make_pair( 100, 0 );
    }

    int roll = 100; // chance of success (%)
    int risk = 0;   // chance of failure (%)
    int chances = 1; // start with 1 in 6 (~17% chance)

    for( const auto &e : mod.type->min_skills ) {
        // gain an additional chance for every level above the minimum requirement
        skill_id sk = e.first.str() == "weapon" ? gun.gun_skill() : e.first;
        chances += std::max( get_skill_level( sk ) - e.second, 0 );
    }
    // cap success from skill alone to 1 in 5 (~83% chance)
    roll = std::min( static_cast<double>( chances ), 5.0 ) / 6.0 * 100;
    // focus is either a penalty or bonus of at most +/-10%
    roll += ( std::min( std::max( get_focus(), 140 ), 60 ) - 100 ) / 4;
    // dexterity and intelligence give +/-2% for each point above or below 12
    roll += ( get_dex() - 12 ) * 2;
    roll += ( get_int() - 12 ) * 2;
    // each level of damage to the base gun reduces success by 10%
    roll -= std::max( gun.damage_level(), 0 ) * 10;
    roll = std::min( std::max( roll, 0 ), 100 );

    // risk of causing damage on failure increases with less durable guns
    risk = ( 100 - roll ) * ( ( 10.0 - std::min( gun.type->gun->durability, 9 ) ) / 10.0 );

    return std::make_pair( roll, risk );
}

void Character::gunmod_add( item &gun, item &mod )
{
    if( !gun.is_gunmod_compatible( mod ).success() ) {
        debugmsg( "Tried to add incompatible gunmod" );
        return;
    }

    if( !has_item( gun ) && !has_item( mod ) ) {
        debugmsg( "Tried gunmod installation but mod/gun not in player possession" );
        return;
    }

    // first check at least the minimum requirements are met
    if( !has_trait( trait_DEBUG_HS ) && !can_use( mod, gun ) ) {
        return;
    }

    // any (optional) tool charges that are used during installation
    auto odds = gunmod_installation_odds( gun, mod );
    int roll = odds.first;
    int risk = odds.second;

    std::string tool;
    int qty = 0;

    if( mod.is_irremovable() ) {
        if( !query_yn( _( "Permanently install your %1$s in your %2$s?" ),
                       colorize( mod.tname(), mod.color_in_inventory() ),
                       colorize( gun.tname(), gun.color_in_inventory() ) ) ) {
            add_msg_if_player( _( "Never mind." ) );
            return; // player canceled installation
        }
    }

    // if chance of success <100% prompt user to continue
    if( roll < 100 ) {
        uilist prompt;
        prompt.text = string_format( _( "Attach your %1$s to your %2$s?" ), mod.tname(),
                                     gun.tname() );

        std::vector<std::function<void()>> actions;

        prompt.addentry( -1, true, 'w',
                         string_format( _( "Try without tools (%i%%) risking damage (%i%%)" ), roll, risk ) );
        actions.emplace_back( [&] {} );

        prompt.addentry( -1, has_charges( itype_small_repairkit, 100 ), 'f',
                         string_format( _( "Use 100 charges of firearm repair kit (%i%%)" ), std::min( roll * 2, 100 ) ) );

        actions.emplace_back( [&] {
            tool = "small_repairkit";
            qty = 100;
            roll *= 2; // firearm repair kit improves success...
            risk /= 2; // ...and reduces the risk of damage upon failure
        } );

        prompt.addentry( -1, has_charges( itype_large_repairkit, 25 ), 'g',
                         string_format( _( "Use 25 charges of gunsmith repair kit (%i%%)" ), std::min( roll * 3, 100 ) ) );

        actions.emplace_back( [&] {
            tool = "large_repairkit";
            qty = 25;
            roll *= 3; // gunsmith repair kit improves success markedly...
            risk = 0;  // ...and entirely prevents damage upon failure
        } );

        prompt.query();
        if( prompt.ret < 0 ) {
            add_msg_if_player( _( "Never mind." ) );
            return; // player canceled installation
        }
        actions[prompt.ret]();
    }

    const int moves = !has_trait( trait_DEBUG_HS ) ? mod.type->gunmod->install_time : 0;

    assign_activity( activity_id( "ACT_GUNMOD_ADD" ), moves, -1, 0, tool );
    activity.targets.emplace_back( *this, &gun );
    activity.targets.emplace_back( *this, &mod );
    activity.values.push_back( 0 ); // dummy value
    activity.values.push_back( roll ); // chance of success (%)
    activity.values.push_back( risk ); // chance of damage (%)
    activity.values.push_back( qty ); // tool charges
}

bool Character::gunmod_remove( item &gun, item &mod )
{
    std::vector<item *> mods = gun.gunmods();
    size_t gunmod_idx = mods.size();
    for( size_t i = 0; i < mods.size(); i++ ) {
        if( mods[i] == &mod ) {
            gunmod_idx = i;
            break;
        }
    }
    if( gunmod_idx == mods.size() ) {
        debugmsg( "Cannot remove non-existent gunmod" );
        return false;
    }

    if( !gunmod_remove_activity_actor::gunmod_unload( *this, mod ) ) {
        return false;
    }

    // Removing gunmod takes only half as much time as installing it
    const int moves = has_trait( trait_DEBUG_HS ) ? 0 : mod.type->gunmod->install_time / 2;
    item_location gun_loc = item_location( *this, &gun );
    assign_activity(
        player_activity(
            gunmod_remove_activity_actor( moves, gun_loc, static_cast<int>( gunmod_idx ) ) ) );
    return true;
}

bool Character::has_gun_for_ammo( const ammotype &at ) const
{
    return has_item_with( [at]( const item & it ) {
        // item::ammo_type considers the active gunmod.
        return it.is_gun() && it.ammo_types().count( at );
    } );
}

bool Character::has_magazine_for_ammo( const ammotype &at ) const
{
    return has_item_with( [&at]( const item & it ) {
        return !it.has_flag( flag_NO_RELOAD ) &&
               ( ( it.is_magazine() && it.ammo_types().count( at ) ) ||
                 ( it.is_gun() && it.magazine_integral() && it.ammo_types().count( at ) ) ||
                 ( it.is_gun() && it.magazine_current() != nullptr &&
                   it.magazine_current()->ammo_types().count( at ) ) );
    } );
}

