#include "veh_utils.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <list>
#include <map>
#include <utility>
#include <vector>

#include "calendar.h"
#include "character.h"
#include "craft_command.h"
#include "enums.h"
#include "game_constants.h"
#include "input_context.h"
#include "inventory.h"
#include "item.h"
#include "map.h"
#include "point.h"
#include "requirements.h"
#include "translations.h"
#include "ui.h"
#include "units_fwd.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"

namespace veh_utils
{

int calc_xp_gain( const vpart_info &vp, const skill_id &sk, const Character &who )
{
    const auto iter = vp.install_skills.find( sk );
    if( iter == vp.install_skills.end() ) {
        return 0;
    }

    // how many levels are we above the requirement?
    const int lvl = std::max( static_cast<int>( who.get_skill_level( sk ) ) - iter->second, 1 );

    // scale xp gain per hour according to relative level
    // 0-1: 60 xp /h
    //   2: 15 xp /h
    //   3:  6 xp /h
    //   4:  4 xp /h
    //   5:  3 xp /h
    //   6:  2 xp /h
    //  7+:  1 xp /h
    return std::ceil( to_moves<double>( vp.install_moves ) /
                      to_moves<int>( 1_minutes * std::pow( lvl, 2 ) ) );
}

vehicle_part *most_repairable_part( vehicle &veh, Character &who )
{
    const inventory &inv = who.crafting_inventory();
    vehicle_part *vp_broken = nullptr;
    vehicle_part *vp_most_damaged = nullptr;
    int most_damage = 0;
    for( const vpart_reference &vpr : veh.get_all_parts() ) {
        vehicle_part &vp = vpr.part();
        const vpart_info &info = vpr.info();
        if( vp.removed
            || !vp.is_repairable()
            || veh.would_repair_prevent_flyable( vp, who ) ) {
            continue;
        }

        if( vp.is_broken() ) {
            if( who.meets_skill_requirements( info.install_skills ) &&
                info.install_requirements().can_make_with_inventory( inv, is_crafting_component ) ) {
                vp_broken = &vp;
            }
            continue;
        }

        if( who.meets_skill_requirements( info.repair_skills ) ) {
            const requirement_data reqs = info.repair_requirements() * vp.get_base().repairable_levels();
            if( reqs.can_make_with_inventory( inv, is_crafting_component ) ) {
                const int repairable_damage = vp.get_base().damage();
                if( repairable_damage > most_damage ) {
                    most_damage = repairable_damage;
                    vp_most_damaged = &vp;
                }
            }
        }
    }
    return vp_most_damaged != nullptr ? vp_most_damaged : vp_broken;
}

bool repair_part( vehicle &veh, vehicle_part &pt, Character &who )
{
    const vpart_info &vp = pt.info();

    const requirement_data reqs = pt.is_broken()
                                  ? vp.install_requirements()
                                  : vp.repair_requirements() * pt.get_base().repairable_levels();

    const inventory &inv = who.crafting_inventory( who.pos(), PICKUP_RANGE, !who.is_npc() );
    inventory map_inv;
    // allow NPCs to use welding rigs they can't see ( on the other side of a vehicle )
    // as they have the handicap of not being able to use the veh interaction menu
    // or able to drag a welding cart etc.
    map_inv.form_from_map( who.pos(), PICKUP_RANGE, &who, false, !who.is_npc() );
    if( !reqs.can_make_with_inventory( inv, is_crafting_component ) ) {
        who.add_msg_if_player( m_info, _( "You don't meet the requirements to repair the %s." ),
                               pt.name() );
        return false;
    }

    // consume items extracting any base item (which we will need if replacing broken part)
    item base( vp.base_item );
    for( const auto &e : reqs.get_components() ) {
        for( item &obj : who.consume_items( who.select_item_component( e, 1, map_inv ), 1,
                                            is_crafting_component ) ) {
            if( obj.typeId() == vp.base_item ) {
                base = obj;
            }
        }
    }

    for( const auto &e : reqs.get_tools() ) {
        who.consume_tools( who.select_tool_component( e, 1, map_inv ), 1 );
    }

    who.invalidate_crafting_inventory();

    for( const auto &sk : pt.is_broken() ? vp.install_skills : vp.repair_skills ) {
        who.practice( sk.first, calc_xp_gain( vp, sk.first, who ) );
    }

    // If part is broken, it will be destroyed and references invalidated
    const std::string partname = pt.name( false );
    const std::string startdurability = pt.get_base().damage_indicator();
    if( pt.is_broken() ) {
        const vpart_id vpid = pt.info().id;
        const point mount = pt.mount;
        const units::angle direction = pt.direction;
        const std::string variant = pt.variant;
        get_map().spawn_items( who.pos(), pt.pieces_for_broken_part() );
        veh.remove_part( pt );
        const int partnum = veh.install_part( mount, vpid, std::move( base ) );
        if( partnum >= 0 ) {
            vehicle_part &vp = veh.part( partnum );
            vp.direction = direction;
            vp.variant = variant;
        }
        veh.part_removal_cleanup();
        who.add_msg_if_player( m_good, _( "You replace the %1$s's %2$s. (was %3$s)" ),
                               veh.name, partname, startdurability );
    } else {
        veh.set_hp( pt, pt.info().durability, true );
        who.add_msg_if_player( m_good, _( "You repair the %1$s's %2$s. (was %3$s)" ),
                               veh.name, partname, startdurability );
    }
    return true;
}

} // namespace veh_utils

veh_menu_item &veh_menu_item::text( const std::string &text )
{
    this->_text = text;
    return *this;
}

veh_menu_item &veh_menu_item::desc( const std::string &desc )
{
    this->_desc = desc;
    return *this;
}

veh_menu_item &veh_menu_item::enable( const bool enable )
{
    this->_enabled = enable;
    return *this;
}

veh_menu_item &veh_menu_item::skip_theft_check( const bool skip_theft_check )
{
    this->_check_theft = !skip_theft_check;
    return *this;
}

veh_menu_item &veh_menu_item::skip_locked_check( const bool skip_locked_check )
{
    this->_check_locked = !skip_locked_check;
    return *this;
}

static std::optional<input_event> veh_keybind( const std::optional<std::string> &hotkey )
{
    if( !hotkey.has_value() || hotkey->empty() ) {
        return std::nullopt;
    }

    const std::vector<input_event> hk = input_context( "VEHICLE" )
                                        .keys_bound_to( *hotkey, /* maximum_modifier_count = */ 1 );
    if( !hk.empty() ) {
        return hk.front();
    }

    return input_event();
}

veh_menu_item &veh_menu_item::hotkey( const char hotkey_char )
{
    this->_hotkey_action = std::nullopt;
    this->_hotkey_char = hotkey_char;
    return *this;
}

veh_menu_item &veh_menu_item::hotkey( const std::string &action )
{
    this->_hotkey_action = action;
    this->_hotkey_char = std::nullopt;
    return *this;
}

veh_menu_item &veh_menu_item::hotkey_auto()
{
    this->_hotkey_char = std::nullopt;
    this->_hotkey_action = std::nullopt;
    return *this;
}

veh_menu_item &veh_menu_item::on_submit( const std::function<void()> &on_submit )
{
    this->_on_submit = on_submit;
    return *this;
}

veh_menu_item &veh_menu_item::keep_menu_open( const bool keep_menu_open )
{
    this->_keep_menu_open = keep_menu_open;
    return *this;
}

veh_menu_item &veh_menu_item::location( const std::optional<tripoint> &location )
{
    this->_location = location;
    return *this;
}

veh_menu::veh_menu( vehicle &veh, const std::string &title ): veh( veh )
{
    this->title = title;
}

veh_menu::veh_menu( vehicle *veh, const std::string &title ): veh( *veh )
{
    this->title = title;
}

veh_menu_item &veh_menu::add( const std::string &txt )
{
    veh_menu_item item;
    item._text = txt;
    this->items.push_back( item );
    return items.back();
}

size_t veh_menu::get_items_size() const
{
    return items.size();
}

std::vector<veh_menu_item> veh_menu::get_items() const
{
    return items;
}

std::vector<tripoint> veh_menu::get_locations() const
{
    std::vector<tripoint> locations;

    for( const veh_menu_item &it : items ) {
        if( it._location.has_value() ) {
            locations.push_back( it._location.value() );
        }
    }

    return locations;
}

void veh_menu::reset( bool keep_last_selected )
{
    last_selected = keep_last_selected ? last_selected : 0;
    items.clear();
}

std::vector<uilist_entry> veh_menu::get_uilist_entries() const
{
    std::vector<uilist_entry> entries;

    for( size_t i = 0; i < items.size(); i++ ) {
        const veh_menu_item &it = items[i];
        uilist_entry entry( it._text, std::nullopt );
        if( it._hotkey_action.has_value() ) {
            entry = uilist_entry( it._text, veh_keybind( it._hotkey_action ) );
        } else if( it._hotkey_char.has_value() ) {
            entry = uilist_entry( it._text, it._hotkey_char.value() );
        }

        entry.retval = static_cast<int>( i );
        entry.desc = it._desc;
        entry.enabled = it._enabled;

        if( it._check_locked && veh.is_locked ) {
            entry.enabled = false;
            if( !entry.desc.empty() ) {
                entry.desc += "\n";
            }
            entry.desc += _( "Vehicle is locked." );
        }

        entries.push_back( entry );
    }

    return entries;
}

bool veh_menu::query()
{
    if( items.empty() ) {
        debugmsg( "veh_menu::query() called with empty items" );
        return false;
    }

    uilist menu;

    menu.input_category = "VEHICLE";
    for( const veh_menu_item &it : items ) {
        if( it._hotkey_action.has_value() ) {
            menu.additional_actions.emplace_back( it._hotkey_action.value(), translation() );
        }
    }

    menu.title = title;
    menu.entries = get_uilist_entries();
    menu.desc_lines_hint = desc_lines_hint;
    menu.desc_enabled = std::any_of( menu.entries.begin(), menu.entries.end(),
    []( const uilist_entry & it ) {
        return !it.desc.empty();
    } );
    menu.hilight_disabled = true;

    const std::vector<tripoint> locations = get_locations();
    pointmenu_cb callback( locations );
    if( locations.size() == items.size() ) { // all items have valid location attached
        menu.callback = &callback;
        menu.w_x_setup = 4; // move menu to the left so more space around vehicle is visible
    } else {
        menu.callback = nullptr;
    }

    menu.selected = last_selected;
    menu.query();
    last_selected = menu.selected;

    const size_t index = static_cast<size_t>( menu.ret );
    if( index >= items.size() ) {
        return false;
    }

    const veh_menu_item &chosen = items[index];
    if( chosen._check_theft && !veh.handle_potential_theft( get_player_character() ) ) {
        return false;
    }

    if( !chosen._on_submit ) {
        debugmsg( "selected veh_menu_item has no attached std::function" );
        return false;
    }

    chosen._on_submit();

    veh.refresh();
    map &m = get_map();
    m.invalidate_map_cache( m.get_abs_sub().z() );

    return chosen._keep_menu_open;
}
