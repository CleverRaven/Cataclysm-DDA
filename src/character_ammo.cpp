#include "ammo.h"

#include <utility>

#include "character.h"
#include "character_modifier.h"
#include "flag.h"
#include "game.h"
#include "itype.h"
#include "output.h"

static const character_modifier_id character_modifier_reloading_move_mod( "reloading_move_mod" );
static const skill_id skill_gun( "gun" );

int Character::ammo_count_for( const item_location &gun ) const
{
    int ret = item::INFINITE_CHARGES;
    if( !gun || !gun->is_gun() ) {
        return ret;
    }

    int required = gun->ammo_required();

    if( required > 0 ) {
        int total_ammo = 0;
        total_ammo += gun->ammo_remaining();

        bool has_mag = gun->magazine_integral();

        const auto found_ammo = find_ammo( *gun, true, -1 );
        int loose_ammo = 0;
        for( const item_location &ammo : found_ammo ) {
            if( ammo->is_magazine() ) {
                has_mag = true;
                total_ammo += ammo->ammo_remaining();
            } else if( ammo->is_ammo() ) {
                loose_ammo += ammo->charges;
            }
        }

        if( has_mag ) {
            total_ammo += loose_ammo;
        }

        ret = std::min( ret, total_ammo / required );
    }

    units::energy energy_drain = gun->get_gun_energy_drain();
    if( energy_drain > 0_kJ ) {
        ret = std::min( ret, static_cast<int>( gun->energy_remaining( this ) / energy_drain ) );
    }

    return ret;
}

bool Character::can_reload( const item &it, const item *ammo ) const
{
    if( ammo && !it.can_reload_with( *ammo, false ) ) {
        return false;
    }

    if( it.is_ammo_belt() ) {
        const std::optional<itype_id> &linkage = it.type->magazine->linkage;
        if( linkage && !has_charges( *linkage, 1 ) ) {
            return false;
        }
    }

    return true;
}

bool Character::list_ammo( const item_location &base, std::vector<item::reload_option> &ammo_list,
                           bool empty ) const
{
    // Associate the destination with "parent"
    // Useful for handling gun mods with magazines
    std::vector<item_location> opts;
    opts.emplace_back( base );

    if( base->magazine_current() ) {
        opts.emplace_back( base, const_cast<item *>( base->magazine_current() ) );
    }

    for( const item *mod : base->gunmods() ) {
        item_location mod_loc( base, const_cast<item *>( mod ) );
        opts.emplace_back( mod_loc );
        if( mod->magazine_current() ) {
            opts.emplace_back( mod_loc, const_cast<item *>( mod->magazine_current() ) );
        }
    }

    bool ammo_match_found = false;
    int ammo_search_range = is_mounted() ? -1 : 1;
    for( item_location &p : opts ) {
        for( item_location &ammo : find_ammo( *p, empty, ammo_search_range ) ) {
            if( p->can_reload_with( *ammo.get_item(), false ) ) {
                // Record that there's a matching ammo type,
                // even if something is preventing reloading at the moment.
                ammo_match_found = true;
            } else if( ( ammo->has_flag( flag_SPEEDLOADER ) || ammo->has_flag( flag_SPEEDLOADER_CLIP ) ) &&
                       p->allows_speedloader( ammo->typeId() ) && ammo->ammo_remaining() > 1 && p->ammo_remaining() < 1 ) {
                // Again, this is "are they compatible", later check handles "can we do it now".
                ammo_match_found = p->can_reload_with( *ammo.get_item(), false );
            }
            if( can_reload( *p, ammo.get_item() ) ) {
                ammo_list.emplace_back( this, p, std::move( ammo ) );
            }
        }
    }
    return ammo_match_found;
}

int Character::item_reload_cost( const item &it, const item &ammo, int qty ) const
{
    if( ammo.is_ammo() || ammo.is_frozen_liquid() || ammo.made_of_from_type( phase_id::LIQUID ) ) {
        qty = std::max( std::min( ammo.charges, qty ), 1 );
    } else if( ammo.is_ammo_container() ) {
        int min_clamp = 0;
        // find the first ammo in the container to get its charges
        ammo.visit_items( [&min_clamp]( const item * it, item * ) {
            if( it->is_ammo() ) {
                min_clamp = it->charges;
                return VisitResponse::ABORT;
            }
            return VisitResponse::NEXT;
        } );

        qty = clamp( qty, min_clamp, 1 );
    } else {
        // Handle everything else as magazines
        qty = 1;
    }

    // No base cost for handling ammo - that's already included in obtain cost
    // We have the ammo in our hands right now
    int mv = item_handling_cost( ammo, true, 0, qty );

    if( ammo.has_flag( flag_MAG_BULKY ) ) {
        mv *= 1.5; // bulky magazines take longer to insert
    }

    if( !it.is_gun() && !it.is_magazine() ) {
        return mv + 100; // reload a tool or sealable container
    }

    /** @EFFECT_GUN decreases the time taken to reload a magazine */
    /** @EFFECT_PISTOL decreases time taken to reload a pistol */
    /** @EFFECT_SMG decreases time taken to reload an SMG */
    /** @EFFECT_RIFLE decreases time taken to reload a rifle */
    /** @EFFECT_SHOTGUN decreases time taken to reload a shotgun */
    /** @EFFECT_LAUNCHER decreases time taken to reload a launcher */

    int cost = 0;
    if( it.is_gun() ) {
        cost = it.get_reload_time();
    } else if( it.type->magazine ) {
        cost = it.type->magazine->reload_time * qty;
    } else {
        cost = it.obtain_cost( ammo );
    }

    skill_id sk = it.is_gun() ? it.type->gun->skill_used : skill_gun;
    mv += cost / ( 1.0f + std::min( get_skill_level( sk ) * 0.1f, 1.0f ) );

    if( it.has_flag( flag_STR_RELOAD ) ) {
        /** @EFFECT_STR reduces reload time of some weapons */
        mv -= get_str() * 20;
    }

    return std::max( static_cast<int>( std::round( mv * get_modifier(
                                           character_modifier_reloading_move_mod ) ) ), 25 );
}

std::vector<item_location> Character::find_reloadables()
{
    std::vector<item_location> reloadables;

    visit_items( [this, &reloadables]( item * node, item * ) {
        if( node->is_reloadable() ) {
            reloadables.emplace_back( *this, node );
        }
        return VisitResponse::NEXT;
    } );
    return reloadables;
}

hint_rating Character::rate_action_reload( const item &it ) const
{
    if( !it.is_reloadable() ) {
        return hint_rating::cant;
    }

    return can_reload( it ) ? hint_rating::good : hint_rating::iffy;
}

hint_rating Character::rate_action_unload( const item &it ) const
{
    if( it.is_container() && !it.empty() &&
        it.can_unload_liquid() ) {
        return hint_rating::good;
    }

    if( it.has_flag( flag_NO_UNLOAD ) ) {
        return hint_rating::cant;
    }

    if( it.magazine_current() ) {
        return hint_rating::good;
    }

    for( const item *e : it.gunmods() ) {
        if( ( e->is_gun() && !e->has_flag( flag_NO_UNLOAD ) &&
              ( e->magazine_current() || e->ammo_remaining() > 0 || e->casings_count() > 0 ) ) ||
            ( e->has_flag( flag_BRASS_CATCHER ) && !e->is_container_empty() ) ) {
            return hint_rating::good;
        }
    }

    if( it.ammo_types().empty() ) {
        return hint_rating::cant;
    }

    if( it.ammo_remaining() > 0 || it.casings_count() > 0 ) {
        return hint_rating::good;
    }

    return hint_rating::iffy;
}

hint_rating Character::rate_action_insert( const item_location &loc ) const
{
    if( loc->will_spill_if_unsealed() && loc.where() != item_location::type::map  &&
        !is_wielding( *loc ) ) {
        return hint_rating::cant;
    }
    return hint_rating::good;
}

