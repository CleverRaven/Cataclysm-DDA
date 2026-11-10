#include "activity_handlers.h"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

#include "activity_actor_definitions.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "cuboid_rectangle.h"
#include "debug.h"
#include "enums.h"
#include "flag.h"
#include "game_constants.h"
#include "game_inventory.h"
#include "iexamine.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "map.h"
#include "mapdata.h"
#include "messages.h"
#include "monster.h"
#include "overmap_ui.h"
#include "pathfinding.h"
#include "player_activity.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "skill.h"
#include "string_formatter.h"
#include "temp_crafting_inventory.h"
#include "translations.h"
#include "type_id.h"
#include "uilist.h"
#include "units.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "weather.h"

static const activity_id ACT_FILL_LIQUID( "ACT_FILL_LIQUID" );
static const activity_id ACT_TRAVELLING( "ACT_TRAVELLING" );

static const ammotype ammo_battery( "battery" );

static const flag_id json_flag_IRREMOVABLE( "IRREMOVABLE" );
static const flag_id json_flag_PSEUDO( "PSEUDO" );

static const furn_str_id furn_f_compost_empty( "f_compost_empty" );
static const furn_str_id furn_f_compost_full( "f_compost_full" );
static const furn_str_id furn_f_fvat_empty( "f_fvat_empty" );
static const furn_str_id furn_f_fvat_wood_empty( "f_fvat_wood_empty" );
static const furn_str_id furn_f_fvat_wood_full( "f_fvat_wood_full" );

static const itype_id itype_battery( "battery" );
static const itype_id itype_pseudo_magazine( "pseudo_magazine" );
static const itype_id itype_pseudo_magazine_mod( "pseudo_magazine_mod" );

using namespace activity_handlers;

const std::map< activity_id, std::function<void( player_activity *, Character * )> >
activity_handlers::do_turn_functions = {
    { ACT_FILL_LIQUID, fill_liquid_do_turn },
    { ACT_TRAVELLING, travel_do_turn }
};

bool activity_handlers::resume_for_multi_activities( Character &you )
{
    if( !you.backlog.empty() ) {
        player_activity &back_act = you.backlog.front();
        if( back_act.is_multi_type() ) {
            you.assign_activity( back_act );
            return true;
        }
    }
    return false;
}

void activity_handlers::fill_liquid_do_turn( player_activity *act, Character *you )
{
    player_activity &act_ref = *act;
    try {
        // 1. Gather the source item.
        vehicle *source_veh = nullptr;
        map &here = get_map();
        const tripoint_bub_ms source_pos = here.get_bub( act_ref.coords.at( 0 ) );
        map_stack source_stack = here.i_at( source_pos );
        map_stack::iterator on_ground;
        monster *source_mon = nullptr;
        item liquid;
        const liquid_source_type source_type = static_cast<liquid_source_type>( act_ref.values.at( 0 ) );
        int part_num = -1;
        int veh_charges = 0;
        switch( source_type ) {
            case liquid_source_type::VEHICLE:
                source_veh = veh_pointer_or_null( here.veh_at( source_pos ) );
                if( source_veh == nullptr ) {
                    throw std::runtime_error( "could not find source vehicle for liquid transfer" );
                }
                deserialize_from_string( liquid, act_ref.str_values.at( 0 ) );
                part_num = static_cast<int>( act_ref.values.at( 1 ) );
                veh_charges = liquid.charges;
                break;
            case liquid_source_type::INFINITE_MAP:
                deserialize_from_string( liquid, act_ref.str_values.at( 0 ) );
                liquid.charges = item::INFINITE_CHARGES;
                break;
            case liquid_source_type::MAP_ITEM:
                if( static_cast<size_t>( act_ref.values.at( 1 ) ) >= source_stack.size() ) {
                    throw std::runtime_error( "could not find source item on ground for liquid transfer" );
                }
                on_ground = source_stack.begin();
                std::advance( on_ground, act_ref.values.at( 1 ) );
                liquid = *on_ground;
                break;
            case liquid_source_type::MONSTER:
                Creature *c = get_creature_tracker().creature_at( source_pos );
                source_mon = dynamic_cast<monster *>( c );
                if( source_mon == nullptr ) {
                    debugmsg( "could not find source creature for liquid transfer" );
                    act_ref.set_to_null();
                }
                deserialize_from_string( liquid, act_ref.str_values.at( 0 ) );
                liquid.charges = 1;
                break;
        }

        // Vehicle siphoning is slower than other liquid transfers
        const units::volume volume_per_second = source_type == liquid_source_type::VEHICLE
                                                ? units::from_liter( 0.25F )
                                                : units::from_liter( 4.0F / 6.0F );
        const int charges_per_second = std::max( 1, liquid.charges_per_volume( volume_per_second ) );
        liquid.charges = std::min( charges_per_second, liquid.charges );
        const int original_charges = liquid.charges;
        if( liquid.has_temperature() && units::to_joule_per_gram( liquid.specific_energy ) < 0 ) {
            liquid.set_item_temperature( std::max( get_weather().get_temperature( you->pos_bub() ),
                                                   temperatures::cold ) );
        }

        // 2. Transfer charges.
        const vehicle *veh = nullptr;
        size_t part;
        switch( static_cast<liquid_target_type>( act_ref.values.at( 2 ) ) ) {
            case liquid_target_type::VEHICLE: {
                const optional_vpart_position vp = here.veh_at( here.get_bub( act_ref.coords.at( 1 ) ) );
                if( act_ref.values.size() > 4 && vp ) {
                    const vpart_reference vpr( vp->vehicle(), act_ref.values[4] );
                    veh = &vp->vehicle();
                    part = act_ref.values[4];
                    if( source_veh &&
                        source_veh->fuel_left( here, liquid.typeId(), ( veh ? std::function<bool( const vehicle_part & )> { [&]( const vehicle_part & pa )
                {
                    return &veh->part( part ) != &pa;
                    }
                                                                                                                          } : return_true<const vehicle_part &> ) ) <= 0 ) {
                        act_ref.set_to_null();
                        return;
                    }
                    you->pour_into( vpr, liquid );

                } else {
                    throw std::runtime_error( "could not find target vehicle for liquid transfer" );
                }
                break;
            }
            case liquid_target_type::CONTAINER:
                you->pour_into( act_ref.targets.at( 0 ), liquid, true, true );
                break;
            case liquid_target_type::MAP:
                if( iexamine::has_keg( here.get_bub( act_ref.coords.at( 1 ) ) ) ) {
                    iexamine::pour_into_keg( here.get_bub( act_ref.coords.at( 1 ) ), liquid, false );
                } else {
                    here.add_item_or_charges( here.get_bub( act_ref.coords.at( 1 ) ), liquid );
                    you->add_msg_if_player( _( "You pour %1$s onto the ground." ), liquid.tname() );
                    liquid.charges = 0;
                }
                break;
            case liquid_target_type::MONSTER:
                liquid.charges = 0;
                break;
        }

        const int removed_charges = original_charges - liquid.charges;
        if( removed_charges == 0 ) {
            // Nothing has been transferred, target must be full.
            act_ref.set_to_null();
            return;
        }

        // 3. Remove charges from source.
        switch( source_type ) {
            case liquid_source_type::VEHICLE:
                if( part_num != -1 ) {
                    const vehicle_part &pt = source_veh->part( part_num );
                    if( pt.is_leaking() && !pt.ammo_remaining( ) ) {
                        act_ref.set_to_null(); // leaky tank spilled while we were transferring
                        return;
                    }
                    source_veh->drain( here, part_num, removed_charges );
                    liquid.charges = veh_charges - removed_charges;
                    // If there's no liquid left in this tank we're done, otherwise
                    // we need to update our liquid serialization to reflect how
                    // many charges are actually left for the next time we come
                    // around this loop.
                    if( !liquid.charges ) {
                        act_ref.set_to_null();
                    } else {
                        if( act_ref.str_values.empty() ) {
                            act_ref.str_values.emplace_back( );
                        }
                        act_ref.str_values[0] = serialize( liquid );
                    }
                } else {
                    source_veh->drain( here, liquid.typeId(), removed_charges,
                                       ( veh ? std::function<bool( vehicle_part & )> { [&]( vehicle_part & pa )
                    {
                        return &veh->part( part ) != &pa;
                    }
                                                                                     } : return_true<vehicle_part &> ) );
                }
                break;
            case liquid_source_type::MAP_ITEM:
                on_ground->charges -= removed_charges;
                if( on_ground->charges <= 0 ) {
                    source_stack.erase( on_ground );
                    if( here.ter( source_pos )->has_examine( iexamine::gaspump ) ) {
                        add_msg( _( "With a clang and a shudder, the %s pump goes silent." ),
                                 liquid.type_name( 1 ) );
                    } else if( const furn_id &f = here.furn( source_pos ); f->has_examine( iexamine::fvat_full ) ) {
                        add_msg( _( "You squeeze the last drops of %s from the vat." ),
                                 liquid.type_name( 1 ) );
                        map_stack items_here = here.i_at( source_pos );
                        if( items_here.empty() ) {
                            if( f == furn_f_fvat_wood_full ) {
                                here.furn_set( source_pos, furn_f_fvat_wood_empty );
                            } else {
                                here.furn_set( source_pos, furn_f_fvat_empty );
                            }
                        }
                    } else if( f->has_examine( iexamine::compost_full ) ) {
                        add_msg( _( "You squeeze the last drops of %s from the tank." ),
                                 liquid.type_name( 1 ) );
                        map_stack items_here = here.i_at( source_pos );
                        if( items_here.empty() ) {
                            if( f == furn_f_compost_full ) {
                                here.furn_set( source_pos, furn_f_compost_empty );
                            }
                        }
                    } else if( iexamine::has_keg( source_pos ) ) {
                        add_msg( _( "You squeeze the last drops of %1$s from the %2$s." ),
                                 liquid.type_name( 1 ), here.furnname( source_pos ) );
                    }
                    act_ref.set_to_null();
                }
                break;
            case liquid_source_type::INFINITE_MAP:
                // nothing, the liquid source is infinite
                break;
            case liquid_source_type::MONSTER:
                // liquid source charges handled in monexamine::milk_source
                if( liquid.charges == 0 ) {
                    act_ref.set_to_null();
                }
                break;
        }

        if( removed_charges < original_charges ) {
            // Transferred less than the available charges -> target must be full
            act_ref.set_to_null();
        }

    } catch( const std::runtime_error &err ) {
        debugmsg( "error in activity data: \"%s\"", err.what() );
        act_ref.set_to_null();
        return;
    }
}

repair_stage repair_item_activity_actor::repeat_menu(
    const std::string &title, repair_stage last_selection,
    const bool can_refit )
{
    uilist rmenu;
    rmenu.text = title;

    rmenu.addentry( static_cast<int>( repair_stage::ONCE ), true, '1',
                    can_refit ? _( "Attempt to refit or repair once" ) : _( "Attempt to repair once" ) );
    rmenu.addentry( static_cast<int>( repair_stage::FULL ), true, '2',
                    can_refit ? _( "Repeat until refitted and fully repaired" ) : _( "Repeat until fully repaired" ) );
    rmenu.addentry( static_cast<int>( repair_stage::EVENT ), true, '3',
                    can_refit ? _( "Refit or repair until success/failure/level up" ) :
                    _( "Repair until success/failure/level up" ) );
    rmenu.addentry( static_cast<int>( repair_stage::REFIT_ONCE ), can_refit, '4',
                    _( "Attempt to refit once" ) );
    rmenu.addentry( static_cast<int>( repair_stage::REFIT_FULL ), can_refit, '5',
                    _( "Repeat until refitted" ) );
    rmenu.addentry( static_cast<int>( repair_stage::INIT ), true, '6', _( "Back to item selection" ) );

    rmenu.selected = static_cast<int>( last_selection ) - static_cast<int>( repair_stage::ONCE );
    rmenu.query();

    if( rmenu.ret >= static_cast<int>( repair_stage::INIT ) &&
        rmenu.ret <= static_cast<int>( repair_stage::REFIT_FULL ) ) {
        return static_cast<repair_stage>( rmenu.ret );
    }

    return repair_stage::CANCEL;
}

bool weldrig_hack::init( itype_id vp_tool_id, const tripoint_bub_ms &vp_pos )
{
    map &here = get_map();

    const optional_vpart_position vp = here.veh_at( vp_pos );
    if( !vp ) {
        return false;
    }

    itype_id tool_id( vp_tool_id );
    pseudo_tool = item( tool_id, calendar::turn );
    part_with_pseudo_tool = vp->part_with_tool( here, tool_id );
    return part_with_pseudo_tool.has_value();
}

item *weldrig_hack::get_item()
{
    map &here = get_map();

    if( !part_with_pseudo_tool ) {
        return nullptr;
    }
    pseudo_tool.set_flag( json_flag_PSEUDO );
    item mag_mod( itype_pseudo_magazine_mod );
    mag_mod.set_flag( json_flag_IRREMOVABLE );
    if( !pseudo_tool.put_in( mag_mod, pocket_type::MOD ).success() ) {
        debugmsg( "tool %s has no space for a %s, this is likely a bug",
                  pseudo_tool.typeId().str(), mag_mod.type->nname( 1 ) );
    }
    itype_id mag_type;
    if( pseudo_tool.can_link_up() ) {
        mag_type = itype_pseudo_magazine;
    } else {
        mag_type = pseudo_tool.magazine_default();
    }
    item mag( mag_type );
    mag.clear_items(); // no initial ammo
    if( !pseudo_tool.put_in( mag, pocket_type::MAGAZINE_WELL ).success() ) {
        debugmsg( "inserting %s into %s's MAGAZINE_WELL pocket failed",
                  mag.typeId().str(), pseudo_tool.typeId().str() );
        return nullptr;
    }

    pseudo_tool.ammo_set( itype_battery, part_with_pseudo_tool->vehicle().battery_left( here ) );

    return &pseudo_tool;
}

void weldrig_hack::vehicle_drain()
{
    map &here = get_map();
    // no cable loss since all of this is virtual
    part_with_pseudo_tool->vehicle().drain( here, itype_battery, pseudo_tool.ammo_required(),
                                            return_true< vehicle_part &>, false );
}

void repair_item_activity_actor::finish( player_activity &act, Character &who )
{
    item *used_tool = get_repair_tool( act );
    if( !used_tool ) {
        // get_repair_tool sets this activity to null if no valid tool is found
        return;
    }

    const use_function *use_func = used_tool->typeId()->get_use( repair_iuse_actor_name );
    // TODO: De-uglify this block. Something like get_use<iuse_actor_type>() maybe?
    const repair_item_actor *repair_iuse = dynamic_cast<const repair_item_actor *>
                                           ( use_func->get_actor_ptr() );
    if( repair_iuse == nullptr ) {
        debugmsg( "iuse_actor type descriptor and actual type mismatch" );
    }

    // Valid Repeat choice and target, attempt repair.
    if( stage != repair_stage::INIT && repaired_item.valid() ) {
        do_repair( act, who, repair_iuse, *used_tool );
        if( !act ) {
            return;
        }
    }
    if( !repair_iuse->can_use_tool( who, *used_tool, true ) ) {
        act.set_to_null();
        return;
    }

    // target selection and validation.
    while( !repaired_item.valid() ) {
        item_location item_loc = game_menus::inv::repair( who, repair_iuse, used_tool );

        if( item_loc == item_location::nowhere ) {
            who.add_msg_if_player( m_info, _( "Never mind." ) );
            act.set_to_null();
            return;
        }
        if( repair_iuse->can_repair_target( who, *item_loc, true, true ) ) {
            repaired_item = item_loc;
            stage = repair_stage::INIT;
        }
    }

    if( stage == repair_stage::INIT ) {
        init_repair( act, who, repair_iuse, *used_tool );
        if( !act ) {
            return;
        }
    }
    // Otherwise keep retrying
    act.moves_left = repair_iuse->move_cost;
}

void repair_item_activity_actor::do_repair( player_activity &act, Character &who,
        const repair_item_actor *repair_iuse, item &repair_tool )
{
    map &here = get_map();
    if( !repaired_item ) {
        // The item could disappear for various reasons: moved by follower, burned up, eaten by a grue, etc.
        who.add_msg_if_player( m_warning, _( "You can no longer find the item to repair." ) );
        act.set_to_null();
        return;
    }

    // Remember our level: we want to stop retrying on level up
    const int old_level = who.get_skill_level( repair_iuse->used_skill );
    const repair_item_actor::attempt_hint attempt = repair_iuse->repair( who, repair_tool,
            repaired_item, stage == repair_stage::REFIT_ONCE || stage == repair_stage::REFIT_FULL );
    // Warning: The above call to `repair_item_actor::repair` might
    // invalidate the item and the item_location, for example when
    // spilling items from spillable containers. It is therefore
    // important that we don't use `fix_location` in code below
    // here without first checking whether it is still valid.

    // If the item being repaired has been destroyed stop further
    // processing in case the items being used for the repair was
    // contained by the item being repaired, which will result
    // in the tool being invalid
    if( attempt == repair_item_actor::AS_DESTROYED ||
        attempt == repair_item_actor::AS_CANT_USE_TOOL ) {
        act.set_to_null();
        return;
    }

    if( attempt != repair_item_actor::AS_CANT ) {
        const tripoint_bub_ms tool_pos = ( tool_to_repair_with.valid() &&
                                           tool_to_repair_with.where() == item_location::type::map )
                                         ? tool_to_repair_with.pos_bub( here )
                                         : vp_tool->second;
        repair_tool.consume_tool_uses( 1, here, tool_pos, &who );
        w_hack.vehicle_drain();
    }

    // TODO: Allow setting this in the actor
    if( !repair_tool.ammo_sufficient( &who ) ) {
        who.add_msg_if_player( _( "Your %1$s ran out of charges." ), repair_tool.tname() );
        act.set_to_null();
        return;
    }

    // Print message explaining why we stopped
    // But only if we didn't destroy the item (because then it's obvious)
    const bool destroyed = attempt == repair_item_actor::AS_DESTROYED;
    const bool cannot_continue_repair = attempt == repair_item_actor::AS_CANT || destroyed ||
                                        !repaired_item ||
                                        !repair_iuse->can_repair_target( who, *repaired_item, !destroyed, true );
    if( cannot_continue_repair ) {
        // Cannot continue to repair target, select another target.
        // **Warning**: as soon as the item is popped back, it is destroyed and can't be used anymore!
        repaired_item = item_location::nowhere;
    }

    const bool event_happened = attempt == repair_item_actor::AS_FAILURE ||
                                attempt == repair_item_actor::AS_SUCCESS ||
                                old_level != static_cast<int>( who.get_skill_level( repair_iuse->used_skill ) );
    const bool can_refit = !destroyed && !cannot_continue_repair &&
                           repaired_item->has_flag( flag_VARSIZE ) &&
                           !repaired_item->has_flag( flag_FIT );

    const bool need_input =
        ( stage == repair_stage::ONCE ) ||
        ( stage == repair_stage::EVENT && event_happened ) ||
        ( stage == repair_stage::FULL && cannot_continue_repair ) ||
        ( stage == repair_stage::REFIT_ONCE ) ||
        ( stage == repair_stage::REFIT_FULL && !can_refit );
    if( need_input ) {
        stage = repair_stage::INIT;
        if( no_menu ) {
            act.set_to_null();
            return;
        }
    }
}

// Check tool is valid before we query target and Repeat choice.

void repair_item_activity_actor::init_repair( player_activity &act, Character &who,
        const repair_item_actor *repair_iuse, item &repair_tool )
{
    map &here = get_map();

    const bool can_refit = repaired_item->has_flag( flag_VARSIZE ) &&
                           !repaired_item->has_flag( flag_FIT );
    const int level = who.get_skill_level( repair_iuse->used_skill );
    repair_item_actor::repair_type action_type = repair_iuse->default_action( *repaired_item, level );
    if( action_type == repair_item_actor::RT_NOTHING ) {
        who.add_msg_if_player( _( "You won't learn anything more by doing that." ) );
    }

    const std::pair<float, float> chance = repair_iuse->repair_chance( who, *repaired_item,
                                           action_type );
    if( chance.first <= 0.0f ) {
        action_type = repair_item_actor::RT_PRACTICE;
    }

    std::string title = string_format( _( "%s %s\n" ),
                                       repair_item_actor::action_description( action_type ),
                                       repaired_item->tname() );
    ammotype current_ammo;
    std::string ammo_name;
    if( repair_tool.has_flag( flag_USE_UPS ) || repair_tool.has_link_data() ) {
        ammo_name = _( "battery" );
        current_ammo = ammo_battery;
    } else if( repair_tool.has_flag( flag_USES_BIONIC_POWER ) ) {
        ammo_name = _( "bionic power" );

    } else {
        const itype_id ammo_id = repair_tool.ammo_current().is_null()
                                 ? repair_tool.ammo_default() : repair_tool.ammo_current();
        if( const std::optional<ammotype> at = item::ammotype_of( ammo_id ) ) {
            current_ammo = *at;
        }
        ammo_name = item::nname( repair_tool.ammo_current() );
    }

    int ammo_remaining = repair_tool.ammo_remaining();

    std::set<itype_id> valid_entries = repair_iuse->get_valid_repair_materials( *repaired_item );
    const temp_crafting_inventory &crafting_inv = who.crafting_inventory();
    std::function<bool( const item & )> filter;
    if( repaired_item->is_filthy() ) {
        filter = []( const item & component ) {
            return component.allow_crafting_component();
        };
    } else {
        filter = is_crafting_component;
    }
    std::vector<std::string> material_list;
    for( const auto &component_id : valid_entries ) {
        if( item::count_by_charges( component_id ) ) {
            if( crafting_inv.has_charges( component_id, 1 ) ) {
                material_list.push_back( string_format( _( "%s (%d)" ), item::nname( component_id ),
                                                        crafting_inv.charges_of( component_id ) ) );
            }
        } else if( crafting_inv.has_amount( component_id, 1, false, filter ) ) {
            material_list.push_back( string_format( _( "%s (%d)" ), item::nname( component_id ),
                                                    crafting_inv.amount_of( component_id, false ) ) );
        }
    }

    const int per_use = repair_tool.expected_cost_per_use();
    std::string title_uses_count;
    if( vp_tool.has_value() ) {
        int vehicle_battery_level = 0;
        if( w_hack.part_with_pseudo_tool.has_value() ) {
            vehicle_battery_level = w_hack.part_with_pseudo_tool->vehicle().battery_power_level().second;
        }
        title_uses_count = title_uses_count = string_format(
                _( "Charges: <color_light_blue>%d/%d</color> %s (%d per use)\n" ),
                ammo_remaining, vehicle_battery_level, ammo_name, per_use );
    } else {
        ammo_remaining = repair_tool.ammo_remaining_linked( here, &who );
        if( repair_tool.is_tool() && repair_tool.has_flag( flag_USES_NEARBY_AMMO ) ) {
            title_uses_count = string_format( _( "Charges: <color_light_blue>%d</color> %s (%d per use)\n" ),
                                              ammo_remaining, ammo_name, per_use );
        } else {
            title_uses_count = string_format( _( "Charges: <color_light_blue>%d/%d</color> %s (%d per use)\n" ),
                                              ammo_remaining, repair_tool.ammo_capacity( current_ammo, true ), ammo_name, per_use );
        }
    }
    title += title_uses_count;
    title += string_format( _( "Materials available: %s\n" ), string_join( material_list, ", " ) );
    title += string_format( _( "Skill used: <color_light_blue>%s (%d)</color>\n" ),
                            repair_iuse->used_skill.obj().name(), level );
    title += string_format( _( "Success chance: <color_light_blue>%.1f</color>%%\n" ),
                            100.0f * chance.first );
    title += string_format( _( "Damage chance: <color_light_blue>%.1f</color>%%" ),
                            100.0f * chance.second );

    do {
        stage = repeat_menu( title, stage, can_refit );

        if( stage == repair_stage::CANCEL ) {
            act.set_to_null();
            return;
        }
        //repeat_type = repeat;
        // BACK selected, redo target selection next.
        if( stage == repair_stage::INIT ) {
            repaired_item = item_location::nowhere;
            return;
        }
        if( stage == repair_stage::FULL && repaired_item->damage() <= repaired_item->degradation() &&
            !can_refit ) {
            const char *msg = repaired_item->damage_level() > 0 ?
                              _( "Your %s is repaired as much as possible, considering the degradation." ) :
                              _( "Your %s is already fully repaired." );
            who.add_msg_if_player( m_info, msg, repaired_item->tname() );
            stage = repair_stage::INIT;
        }
    } while( stage == repair_stage::INIT );
}

void activity_handlers::travel_do_turn( player_activity *act, Character *you )
{
    if( !you->omt_path.empty() ) {
        you->omt_path.pop_back();
        if( you->omt_path.empty() ) {
            you->add_msg_if_player( m_info, _( "You have reached your destination." ) );
            act->set_to_null();
            ui::omap::force_quit();
            return;
        }
        const tripoint_abs_omt next_omt = you->omt_path.back();
        tripoint_abs_ms waypoint;
        if( you->omt_path.size() == 1 ) {
            // if next omt is the final one, target its midpoint
            waypoint = midpoint( project_bounds<coords::ms>( next_omt ) );
        } else {
            // otherwise target the middle of the edge nearest to our current location
            const tripoint_abs_ms cur_omt_mid = midpoint( project_bounds<coords::ms>
                                                ( you->pos_abs_omt() ) );
            waypoint = clamp( cur_omt_mid, project_bounds<coords::ms>( next_omt ) );
        }
        map &here = get_map();
        tripoint_bub_ms centre_sub = here.get_bub( waypoint );
        const std::vector<tripoint_bub_ms> route_to =
            here.route( *you, pathfinding_target::radius( centre_sub, 2 ) );
        if( !route_to.empty() ) {
            const activity_id act_travel = ACT_TRAVELLING;
            you->set_destination( route_to, player_activity( act_travel ) );
        } else {
            you->add_msg_if_player( m_warning, _( "You cannot reach that destination." ) );
            ui::omap::force_quit();
        }
    } else {
        you->add_msg_if_player( m_info, _( "You have reached your destination." ) );
        ui::omap::force_quit();
    }
    act->set_to_null();
}

void repair_item_activity_actor::do_turn( player_activity &act, Character &who )
{
    // Moves are decremented based on a combination of speed and good vision (not in the dark, farsighted, etc)
    const float exertion_mult = who.exertion_adjusted_move_multiplier( act.exertion_level() );
    const int effective_moves = who.get_moves() / ( who.fine_detail_vision_mod() * exertion_mult );
    if( effective_moves <= act.moves_left ) {
        act.moves_left -= effective_moves;
        who.set_moves( 0 );
    } else {
        who.mod_moves( -act.moves_left * who.fine_detail_vision_mod() );
        act.moves_left = 0;
    }
}

template<typename fn>
static void cleanup_tiles( std::unordered_set<tripoint_abs_ms> &tiles, fn &cleanup )
{
    auto it = tiles.begin();
    map &here = get_map();
    while( it != tiles.end() ) {
        auto current = it++;

        const tripoint_bub_ms &tile_loc = here.get_bub( *current );

        if( cleanup( tile_loc ) ) {
            tiles.erase( current );
        }
    }
}
