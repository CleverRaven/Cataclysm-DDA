#include "veh_utils.h"
#include <algorithm>
#include <map>

#include "calendar.h"
#include "craft_command.h"
#include "vehicle.h"
#include "output.h"
#include "veh_type.h"
#include "player.h"
#include "messages.h"
#include "game.h"

namespace veh_utils
{

int calc_xp_gain( const vpart_info &vp, const skill_id &sk )
{
    return calc_xp_gain( vp, sk, g->u );
}

int calc_xp_gain( const vpart_info &vp, const skill_id &sk, Character &who )
{
    auto iter = vp.install_skills.find( sk );
    if( iter == vp.install_skills.end() ) {
        return 0;
    }

    // how many levels are we above the requirement?
    int lvl = std::max( who.get_skill_level( sk ) - iter->second, 1 );

    // scale xp gain per hour according to relative level
    // 0-1: 60 xp /h
    //   2: 15 xp /h
    //   3:  6 xp /h
    //   4:  4 xp /h
    //   5:  3 xp /h
    //   6:  2 xp /h
    //  7+:  1 xp /h
    return std::ceil( double( vp.install_moves ) / to_moves<int>( 1_minutes * pow( lvl, 2 ) ) );
}

vehicle_part &most_repairable_part( vehicle &veh, const Character &who_c, bool only_repairable )
{
    // @todo: Get rid of this cast after moving relevant functions down to Character
    player &who = ( player & )who_c;
    const auto &inv = who.crafting_inventory();

    enum repairable_status {
        not_repairable = 0,
        need_replacement,
        repairable
    };
    std::map<const vehicle_part *, repairable_status> repairable_cache;
    for( const vehicle_part &part : veh.parts ) {
        const auto &info = part.info();
        repairable_cache[ &part ] = not_repairable;
        if( part.removed || part.damage() <= 0.0f ) {
            continue;
        }

        if( part.is_broken() ) {
            if( info.install_requirements().can_make_with_inventory( inv ) ) {
                repairable_cache[ &part ] = need_replacement;
            }

            continue;
        }

        if( info.is_repairable() &&
            ( info.repair_requirements() * part.damage() ).can_make_with_inventory( inv ) ) {
            repairable_cache[ &part ] = repairable;
        }
    }

    auto part_damage_comparison = [&repairable_cache]( const vehicle_part & a,
    const vehicle_part & b ) {
        return ( repairable_cache[ &b ] > repairable_cache[ &a ] ) ||
               ( repairable_cache[ &b ] == repairable_cache[ &a ] && b.damage() > a.damage() );
    };

    auto high_damage_iterator = std::max_element( veh.parts.begin(),
                                veh.parts.end(),
                                part_damage_comparison );
    if( high_damage_iterator == veh.parts.end() ||
        high_damage_iterator->removed ||
        !high_damage_iterator->info().is_repairable() ||
        ( only_repairable && !repairable_cache[ &( *high_damage_iterator ) ] ) ) {
        static vehicle_part nullpart;
        return nullpart;
    }

    return *high_damage_iterator;
}

bool repair_part( vehicle &veh, vehicle_part &pt, Character &who_c )
{
    // @todo: Get rid of this cast after moving relevant functions down to Character
    player &who = ( player & )who_c;
    int part_index = veh.index_of_part( &pt );
    auto &vp = pt.info();

    // @todo: Expose base part damage somewhere, don't recalculate it here
    const auto reqs = pt.is_broken() ?
                      vp.install_requirements() :
                      vp.repair_requirements() * pt.damage();

    inventory map_inv;
    map_inv.form_from_map( who.pos(), PICKUP_RANGE );
    if( !reqs.can_make_with_inventory( who.crafting_inventory() ) ) {
        who.add_msg_if_player( m_info, _( "You don't meet the requirements to repair the %s." ),
                               pt.name().c_str() );
        return false;
    }

    // consume items extracting any base item (which we will need if replacing broken part)
    item base( vp.item );
    for( const auto &e : reqs.get_components() ) {
        for( auto &obj : who.consume_items( who.select_item_component( e, 1, map_inv ), 1 ) ) {
            if( obj.typeId() == vp.item ) {
                base = obj;
            }
        }
    }

    for( const auto &e : reqs.get_tools() ) {
        who.consume_tools( who.select_tool_component( e, 1, map_inv ), 1 );
    }

    who.invalidate_crafting_inventory();

    for( const auto &sk : pt.is_broken() ? vp.install_skills : vp.repair_skills ) {
        who.practice( sk.first, calc_xp_gain( vp, sk.first ) );
    }

    // If part is broken, it will be destroyed and references invalidated
    std::string partname = pt.name();
    if( pt.is_broken() ) {
        const int dir = pt.direction;
        point loc = pt.mount;
        auto replacement_id = pt.info().get_id();
        veh.break_part_into_pieces( part_index, who.posx(), who.posy() );
        veh.remove_part( part_index );
        const int partnum = veh.install_part( loc.x, loc.y, replacement_id, std::move( base ) );
        veh.parts[partnum].direction = dir;
        veh.part_removal_cleanup();
    } else {
        veh.set_hp( pt, pt.info().durability );
    }

    // @todo: NPC doing that
    who.add_msg_if_player( m_good, _( "You repair the %1$s's %2$s." ), veh.name.c_str(),
                           partname.c_str() );
    return true;
}

}
