#include <cstddef>
#include <initializer_list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>

#include "cached_options.h"
#include "calendar.h"
#include "cata_imgui.h"
#include "cata_utility.h"
#include "character.h"
#include "color.h"
#include "debug.h"
#include "enums.h"
#include "game.h"
#include "handle_liquid.h"
#include "input_enums.h"
#include "inventory.h"
#include "item_location.h"
#include "itype.h"
#include "iuse.h"
#include "map.h"
#include "map_iterator.h"
#include "messages.h"
#include "output.h"
#include "overmapbuffer.h"
#include "player_activity.h"
#include "requirements.h"
#include "ret_val.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "ui_manager.h"
#include "uilist.h"
#include "units.h"
#include "veh_appliance.h"
#include "veh_interact.h"
#include "veh_type.h"
#include "veh_utils.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"

class uilist_impl;

static const activity_id ACT_VEHICLE( "ACT_VEHICLE" );

static const itype_id fuel_type_battery( "battery" );
static const itype_id itype_power_cord( "power_cord" );
static const itype_id itype_wall_wiring( "wall_wiring" );

static const quality_id qual_HOSE( "HOSE" );

static const trait_id trait_DEBUG_HS( "DEBUG_HS" );

static const vpart_id vpart_ap_standing_lamp( "ap_standing_lamp" );

static const vproto_id vehicle_prototype_none( "none" );

static const std::string flag_APPLIANCE( "APPLIANCE" );
static const std::string flag_WIRING( "WIRING" );
static const std::string flag_HALF_CIRCLE_LIGHT( "HALF_CIRCLE_LIGHT" );

// Width of the entire set of windows. 60 is sufficient for
// all tested cases while remaining within the 80x24 limit.
// TODO: make this dynamic in the future.
static const int win_width = 60;

vpart_id vpart_appliance_from_item( const itype_id &item_id )
{
    for( const vpart_info &vpi : vehicles::parts::get_all() ) {
        if( vpi.base_item == item_id && vpi.has_flag( flag_APPLIANCE ) ) {
            return vpi.id;
        }
    }
    debugmsg( "item %s is not base item of any appliance!", item_id.c_str() );
    return vpart_ap_standing_lamp;
}

bool place_appliance( map &here, const tripoint_bub_ms &p, const vpart_id &vpart,
                      const Character &owner, const std::optional<item> &base )
{

    const vpart_info &vpinfo = vpart.obj();
    vehicle *veh = here.add_vehicle( vehicle_prototype_none, p, 0_degrees, 0, 0 );

    if( !veh ) {
        debugmsg( "error constructing appliance" );
        return false;
    }

    veh->add_tag( flag_APPLIANCE );

    int partnum = -1;
    if( base ) {
        item copied = *base;
        if( vpinfo.base_item != copied.typeId() ) {
            // transform the deploying item into what it *should* be before storing it
            copied.convert( vpinfo.base_item );
        }
        partnum = veh->install_part( here, point_rel_ms::zero, vpart, std::move( copied ) );
    } else {
        partnum = veh->install_part( here, point_rel_ms::zero, vpart );
    }
    if( partnum == -1 ) {
        // unrecoverable, failed to be installed somehow
        here.destroy_vehicle( veh );
        return false;
    }
    veh->name = vpart->name();

    veh->last_update = calendar::turn;

    if( vpinfo.base_item == itype_wall_wiring ) {
        veh->add_tag( flag_WIRING );
    }

    // Update the vehicle cache immediately,
    // or the appliance will be invisible for the first couple of turns.
    here.add_vehicle_to_cache( veh );

    // Connect to any neighbouring appliances or wires once
    std::unordered_set<const vehicle *> connected_vehicles;
    for( const tripoint_bub_ms &trip : here.points_in_radius( p, 1 ) ) {
        const optional_vpart_position vp = here.veh_at( trip );
        if( !vp ) {
            continue;
        }
        vehicle &veh_target = vp->vehicle();
        if( veh_target.has_tag( flag_APPLIANCE ) ) {
            if( veh->is_powergrid() && veh_target.is_powergrid() &&
                veh->merge_appliance_into_grid( &here, veh_target ) ) {
                add_msg( _( "You merge it into the adjacent power grid." ) );
                continue;
            }
            if( connected_vehicles.find( &veh_target ) == connected_vehicles.end() ) {
                veh->connect( &here, p, trip );
                connected_vehicles.insert( &veh_target );
            }
        }
    }
    veh->part_removal_cleanup( here );

    // Make some lighting appliances directed
    if( vpinfo.has_flag( flag_HALF_CIRCLE_LIGHT ) && partnum != -1 ) {
        orient_part( here, veh, vpinfo, partnum );
    }
    veh->set_owner( owner );
    return true;
}

player_activity veh_app_interact::run( map &here, vehicle &veh, const point_rel_ms &p )
{
    veh_app_interact ap( veh, p );
    ap.app_loop( here );
    return ap.act;
}

// Registers general appliance actions from keybindings
veh_app_interact::veh_app_interact( vehicle &veh, const point_rel_ms &p )
    : a_point( p ), veh( &veh ), ctxt( "APP_INTERACT", keyboard_mode::keycode )
{
    ctxt.register_directions();
    ctxt.register_action( "REFILL" );
    ctxt.register_action( "SIPHON" );
    ctxt.register_action( "RENAME" );
    ctxt.register_action( "REMOVE" );
    ctxt.register_action( "MERGE" );
    ctxt.register_action( "DISCONNECT_GRID" );
}

// @returns true if a battery part exists on any vehicle connected to veh
static bool has_battery_in_grid( map &here, vehicle *veh )
{
    return !veh->search_connected_batteries( here ).empty();
}

void veh_app_interact::init_ui_windows( map &here )
{
    imenu.reset();
    populate_app_actions( here );

    int height_info = veh->get_printable_fuel_types( here ).size() + 2;

    if( !has_battery_in_grid( here, veh ) ) {
        height_info++;
    }
    if( !veh->batteries.empty() ) {
        height_info++;
    }
    if( !veh->reactors.empty() ) {
        height_info++;
    }
    if( !veh->wind_turbines.empty() ) {
        height_info++;
    }
    if( !veh->solar_panels.empty() ) {
        height_info++;
    }
    if( !veh->water_wheels.empty() ) {
        height_info++;
    }
    if( !veh->alternators.empty() ) {
        height_info++;
    }
    if( !veh->accessories.empty() ) {
        height_info++;
    }
    const int width_info = win_width - 2;
    const int height_input = app_actions.size();
    const int win_height = height_info + 2;
    const int full_height = win_height + height_input;

    // Center the UI
    point topleft( TERMX / 2 - win_width / 2, TERMY / 2 - full_height / 2 );
    w_border = catacurses::newwin( win_height, win_width, topleft );
    //NOLINTNEXTLINE(cata-use-named-point-constants)
    w_info = catacurses::newwin( height_info, width_info, topleft + point( 1, 1 ) );

    // Try to align the imgui window to be below the header.
    // to that end, we have some awkward math translating character positions to screen positions here:
    ImVec2 text_metrics = { ImGui::CalcTextSize( "X" ).x, ImGui::GetTextLineHeight() };
    ImVec2 origin = text_metrics * ImVec2{ static_cast<float>( topleft.x ), static_cast<float>( topleft.y + win_height ) };
    ImVec2 size = text_metrics * ImVec2{ static_cast<float>( win_width ), static_cast<float>( height_input ) };
    imenu.desired_bounds = { origin.x,
                             origin.y,
                             size.x,  // align the width of the input to be the same as the header
                             -1,      // but let uilist choose the height as it pleases.
                           };

    imenu.allow_cancel = true;
    imenu.border_color = c_white;
    imenu.setup();
}

void veh_app_interact::draw_info( map &here )
{
    werase( w_info );

    int row = 0;
    // Fuel indicators
    veh->print_fuel_indicators( here, w_info, point( 0, row ), 0, true, true, true, true );
    row += veh->get_printable_fuel_types( here ).size();

    // Onboard battery power
    if( !veh->batteries.empty() ) {
        std::pair<int, int> battery = veh->battery_power_level( );
        nc_color batt_col = c_yellow;
        if( battery.second > 0 ) {
            batt_col = battery.first == 0 ? c_light_red :
                       battery.first == battery.second ? c_light_green : c_yellow;
        }
        mvwprintz( w_info, point( 0, row ), c_white, _( "Onboard battery power: " ) );
        wprintz( w_info, batt_col, string_format( "%d/%d", battery.first, battery.second ) );
        row++;
    }

    auto print_charge = [this]( const std::string & lbl, units::power rate, int row ) {
        std::string rstr;
        if( rate > 10_kW || rate < -10_kW ) {
            //~ Power in killoWatts. %+4.1f is a 4 digit number with 1 decimal point (ex: 2198.3 kW)
            rstr = string_format( _( "%+4.1f kW" ), units::to_watt( rate ) / 1000.f );
        } else {
            //~ Power in Watts. %+4.1f is a 4 digit number with 1 decimal point (ex: 4737.3 W)
            rstr = string_format( _( "%+4.1f W" ), units::to_milliwatt( rate ) / 1000.f );
        }
        nc_color rcol = rate < 0_W ? c_light_red :
                        rate > 0_W ? c_light_green : c_yellow;
        mvwprintz( w_info, point( 0, row ), c_white, lbl );
        wprintz( w_info, rcol, rstr );
    };

    if( !has_battery_in_grid( here, veh ) ) {
        mvwprintz( w_info, point( 0, row ), c_light_red, _( "Appliance has no connection to a battery." ) );
        row++;
    }

    // Battery power output
    units::power grid_flow = 0_W;
    for( const std::pair<vehicle *const, float> &pair : veh->search_connected_vehicles( here ) ) {
        grid_flow += pair.first->net_battery_charge_rate( here, /* include_reactors = */ true );
    }
    print_charge( _( "Grid battery power flow: " ), grid_flow, row );
    row++;

    // Reactor power output
    if( !veh->reactors.empty() ) {
        const units::power rate = veh->active_reactor_epower( here );
        print_charge( _( "Reactor power output: " ), rate, row );
        row++;
    }

    // Wind power output
    if( !veh->wind_turbines.empty() ) {
        units::power rate = veh->total_wind_epower( here );
        print_charge( _( "Wind power output: " ), rate, row );
        row++;
    }

    // Solar power output
    if( !veh->solar_panels.empty() ) {
        units::power rate = veh->total_solar_epower( here );
        print_charge( _( "Solar power output: " ), rate, row );
        row++;
    }

    // Water power output
    if( !veh->water_wheels.empty() ) {
        units::power rate = veh->total_water_wheel_epower( here );
        print_charge( _( "Water power output: " ), rate, row );
        row++;
    }

    // Alternator power output
    if( !veh->alternators.empty() ) {
        units::power rate = veh->total_alternator_epower( here );
        print_charge( _( "Alternator power output: " ), rate, row );
        row++;
    }

    // Other power output
    if( !veh->accessories.empty() || veh->has_part( "RECHARGE" ) ) {
        units::power rate = veh->total_accessory_epower() + veh->recharge_epower_this_turn;
        print_charge( _( "Appliance power consumption: " ), rate, row );
        row++;
    }

    wnoutrefresh( w_info );
}

bool veh_app_interact::can_refill( )
{
    for( const vpart_reference &vpr : veh->get_all_parts() ) {
        if( vpr.part().can_reload( ) ) {
            return true;
        }
    }
    return false;
}

bool veh_app_interact::can_siphon()
{
    for( const vpart_reference &vpr : veh->get_any_parts( VPFLAG_FLUIDTANK ) ) {
        if( vpr.part().get_base().has_item_with( []( const item & it ) {
        return it.made_of( phase_id::LIQUID );
        } ) ) {
            return get_player_character().has_quality( qual_HOSE );
        }
    }
    return false;
}

// Helper function for selecting a part in the parts list.
// If only one part is available, don't prompt the player.
static vehicle_part *pick_part( const std::vector<vehicle_part *> &parts,
                                const std::string &query_msg )
{
    if( parts.empty() ) {
        return nullptr;
    }
    vehicle_part *pt = parts.front();
    if( parts.size() > 1 ) {
        uilist pt_sel( query_msg, {} );
        for( const vehicle_part *vpr : parts ) {
            std::string enttxt;
            std::string vname = vpr->name();
            if( !vpr->ammo_current().is_null() && vpr->ammo_current() != fuel_type_battery &&
                !vpr->get_base().empty() ) {
                units::volume mult = 250_ml / item::find_type(
                                         vpr->ammo_current() )->stack_size;
                double vcur = to_liter( vpr->ammo_remaining( ) * mult );
                double vmax = to_liter( vpr->ammo_capacity( vpr->get_base().only_item().ammo_type() ) * mult );
                //~ Vehicle part name, capacity (current/max L) and name of contents
                enttxt = string_format( _( "%1$s (%2$.1f/%3$.1fL %4$s)" ), vname, round_up( vcur, 1 ),
                                        round_up( vmax, 1 ), item::nname( vpr->ammo_current() ) );
            } else {
                enttxt = vname;
            }
            pt_sel.addentry( enttxt );
        }
        pt_sel.query();
        int sel = pt_sel.ret;
        if( sel < 0 || static_cast<size_t>( sel ) >= parts.size() ) {
            return nullptr;
        }
        pt = parts[sel];
    }
    return pt;
}

void veh_app_interact::refill( )
{
    std::vector<vehicle_part *> ptlist;
    for( const vpart_reference &vpr : veh->get_all_parts() ) {
        if( vpr.part().can_reload( ) ) {
            ptlist.emplace_back( &vpr.part() );
        }
    }
    vehicle_part *pt = pick_part( ptlist, _( "Which part to refill?" ) );
    if( pt == nullptr ) {
        return;
    }

    auto validate = [&pt]( const item & obj ) {
        if( pt->is_tank() ) {
            if( obj.is_watertight_container() && obj.num_item_stacks() == 1 ) {
                // we are assuming only one pocket here, and it's a liquid so only one item
                return pt->can_reload( obj.only_item() );
            }
        } else if( pt->is_fuel_store() ) {
            bool can_reload = pt->can_reload( obj );
            //check base item for fuel_stores that can take multiple types of ammunition (like the fuel_bunker)
            if( pt->get_base().can_reload_with( obj, true ) ) {
                return true;
            }
            return can_reload;
        }
        return false;
    };

    // Setup the refill activity
    item_location target = g->inv_map_splice( validate, string_format( _( "Refill %s" ), pt->name() ),
                           1 );
    if( target ) {
        act = player_activity( ACT_VEHICLE, 1000, static_cast<int>( 'f' ) );
        act.targets.push_back( target );
        act.str_values.push_back( pt->info().id.str() );
        const point_rel_ms q = veh->coord_translate( pt->mount );
        for( const tripoint_abs_ms &p : veh->get_points( true ) ) {
            act.coord_set.insert( p );
        }
        act.values.push_back( veh->pos_abs().x() + q.x() );
        act.values.push_back( veh->pos_abs().y() + q.y() );
        act.values.push_back( a_point.x() );
        act.values.push_back( a_point.y() );
        act.values.push_back( -a_point.x() );
        act.values.push_back( -a_point.y() );
        act.values.push_back( veh->index_of_part( pt ) );
    }
}

void veh_app_interact::siphon( map &here )
{
    std::vector<vehicle_part *> ptlist;
    for( const vpart_reference &vpr : veh->get_any_parts( VPFLAG_FLUIDTANK ) ) {
        if( vpr.part().get_base().has_item_with( []( const item & it ) {
        return it.made_of( phase_id::LIQUID );
        } ) ) {
            ptlist.emplace_back( &vpr.part() );
        }
    }
    vehicle_part *pt = pick_part( ptlist, _( "Which part to siphon from?" ) );
    if( pt == nullptr ) {
        return;
    }

    // Setup liquid handling activity
    const item &base = pt->get_base();
    const int idx = veh->index_of_part( pt );
    item liquid( base.legacy_front() );
    const int liq_charges = liquid.charges;
    liquid_dest_opt liquid_target;

    if( liquid_handler::handle_liquid( liquid, liquid_target, nullptr, 1, nullptr, veh, idx ) ) {
        veh->drain( here, idx, liq_charges - liquid.charges );
    }
}

void veh_app_interact::rename()
{
    std::string name = string_input_popup()
                       .title( _( "Enter new appliance name:" ) )
                       .width( 20 )
                       .query_string();
    if( !name.empty() ) {
        veh->name = name;
        if( veh->tracking_on ) {
            overmap_buffer.remove_vehicle( veh );
            // Add the vehicle again, this time with the new name
            overmap_buffer.add_vehicle( veh );
        }
    }
}

void veh_app_interact::remove( map &here )
{
    const tripoint_abs_ms a_point_abs( veh->mount_to_tripoint_abs( a_point ) );

    vehicle_part *vp;
    if( auto sel_part = here.veh_at( a_point_abs ).part_with_feature( VPFLAG_APPLIANCE, false ) ) {
        vp = &sel_part->part();
    } else {
        int const part = veh->part_at( veh->coord_translate( a_point ) );
        vp = &veh->part( part >= 0 ? part : 0 );
    }
    const vpart_info &vpinfo = vp->info();
    const requirement_data reqs = vpinfo.removal_requirements();
    Character &you = get_player_character();
    const inventory &inv = you.crafting_inventory();
    std::string msg;
    bool can_remove = reqs.can_make_with_inventory( inv, is_crafting_component );
    if( !can_remove ) {
        msg += _( "Insufficient components/tools!\n" );
        msg += reqs.list_missing();
    }

    time_duration time = vpinfo.removal_time( you );
    if( you.has_trait( trait_DEBUG_HS ) ) {
        can_remove = true;
        time = 1_seconds;
    }

    if( !can_remove ) {
        popup( msg );
        //~ Prompt the player if they want to remove the appliance. %s = appliance name.
    } else if( query_yn( _( "Are you sure you want to take down the %s?" ), veh->name ) ) {
        act = player_activity( ACT_VEHICLE, to_moves<int>( time ), static_cast<int>( 'O' ) );
        act.str_values.push_back( vpinfo.id.str() );
        for( const tripoint_abs_ms &p : veh->get_points( true ) ) {
            act.coord_set.insert( p );
        }
        act.values.push_back( a_point_abs.x() );
        act.values.push_back( a_point_abs.y() );
        act.values.push_back( a_point.x() );
        act.values.push_back( a_point.y() );
        act.values.push_back( -a_point.x() );
        act.values.push_back( -a_point.y() );
        act.values.push_back( veh->index_of_part( vp ) );
    }
}

bool veh_app_interact::can_disconnect()
{
    for( int &i : veh->parts_at_relative( a_point, true ) ) {
        if( veh->part( i ).has_flag( vp_flag::linked_flag ) ||
            veh->part( i ).info().has_flag( "TOW_CABLE" ) ) {
            return false;
        }
    }
    return true;
}

void veh_app_interact::disconnect( map &here )
{
    veh->separate_from_grid( &here, a_point );
    get_player_character().pause();
}

void veh_app_interact::plug( map &here )
{
    const int part = veh->part_at( veh->coord_translate( a_point ) );
    const tripoint_bub_ms pos = veh->bub_part_pos( here, part );
    item cord( itype_power_cord );
    cord.link_to( *veh, a_point, link_state::automatic );
    if( cord.get_use( "link_up" ) ) {
        cord.type->get_use( "link_up" )->call( &get_player_character(), cord, pos );
    }
}

void veh_app_interact::hide()
{
    const int part_idx = veh->part_at( veh->coord_translate( a_point ) );
    vehicle_part &vp = veh->part( part_idx );
    vp.hidden = !vp.hidden;
}

void veh_app_interact::populate_app_actions( map &here )
{
    vehicle_part *vp;
    const tripoint_abs_ms a_point_abs( veh->mount_to_tripoint_abs( a_point ) );
    if( auto sel_part = here.veh_at( a_point_abs ).part_with_feature( VPFLAG_APPLIANCE, false ) ) {
        vp = &sel_part->part();
    } else {
        const int part = veh->part_at( veh->coord_translate( a_point ) );
        vp = &veh->part( part >= 0 ? part : 0 );
    }

    const std::string ctxt_letters = ctxt.get_available_single_char_hotkeys();
    imenu.entries.clear();
    app_actions.clear();

    /******************** General actions ********************/
    // Refill
    app_actions.emplace_back( [this]() {
        refill( );
    } );
    imenu.addentry( -1, can_refill( ), ctxt.keys_bound_to( "REFILL" ).front(),
                    ctxt.get_action_name( "REFILL" ) );
    // Siphon
    app_actions.emplace_back( [&here, this]() {
        siphon( here );
    } );
    imenu.addentry( -1, can_siphon(), ctxt.keys_bound_to( "SIPHON" ).front(),
                    ctxt.get_action_name( "SIPHON" ) );
    // Rename
    app_actions.emplace_back( [this]() {
        rename();
    } );
    imenu.addentry( -1, true, ctxt.keys_bound_to( "RENAME" ).front(),
                    ctxt.get_action_name( "RENAME" ) );
    // Remove
    app_actions.emplace_back( [&here, this]() {
        remove( here );
    } );
    imenu.addentry( -1, veh->can_unmount( *vp, true ).success(), ctxt.keys_bound_to( "REMOVE" ).front(),
                    ctxt.get_action_name( "REMOVE" ) );
    // Plug
    app_actions.emplace_back( [&here, this]() {
        plug( here );
    } );
    imenu.addentry( -1, true, ctxt.keys_bound_to( "PLUG" ).front(),
                    string_format( "%s%s", ctxt.get_action_name( "PLUG" ),
                                   //~ An addendum to Plug In's description, as in: Plug in appliance / merge power grid".
                                   veh->is_powergrid() ? _( " / merge power grid" ) : "" ) );
#if defined(TILES)
    // Hide
    if( use_tiles && vp->info().has_flag( flag_WIRING ) ) {
        app_actions.emplace_back( [this]() {
            hide();
        } );
        imenu.addentry( -1, true, 0, "Hide/Unhide wiring" );
    }
#endif

    if( veh->is_powergrid() && veh->part_count() > 1 && !vp->info().has_flag( VPFLAG_WALL_MOUNTED ) ) {
        // Disconnect from power grid
        app_actions.emplace_back( [&here, this]() {
            disconnect( here );
            veh = nullptr;
        } );
        const bool can_disc = can_disconnect();
        imenu.addentry_desc( -1, can_disc, ctxt.keys_bound_to( "DISCONNECT_GRID" ).front(),
                             ctxt.get_action_name( "DISCONNECT_GRID" ), can_disc ? "" : _( "Remove other cables first" ) );
    }

    /*************** Get part-specific actions ***************/
    veh_menu menu( veh, "IF YOU SEE THIS IT IS A BUG" );
    veh->build_interact_menu( menu, &here, veh->mount_to_tripoint( &here, a_point ), false );
    const std::vector<veh_menu_item> items = menu.get_items();
    for( size_t i = 0; i < items.size(); i++ ) {
        const veh_menu_item &it = items[i];
        const char hotkey = i < ctxt_letters.size() ? ctxt_letters[i] : 0;
        imenu.addentry( -1, it._enabled, hotkey, it._text );
        app_actions.emplace_back( it._on_submit );
    }
}

shared_ptr_fast<ui_adaptor> veh_app_interact::create_or_get_ui_adaptor( map &here )
{
    shared_ptr_fast<ui_adaptor> current_ui = ui.lock();
    if( !current_ui ) {
        ui = current_ui = make_shared_fast<ui_adaptor>();
        current_ui->on_screen_resize( [&here, this]( ui_adaptor & cui ) {
            init_ui_windows( here );
            cui.position_from_window( catacurses::stdscr );
        } );
        current_ui->mark_resize();
        current_ui->on_redraw( [&here, this]( const ui_adaptor & ) {
            draw_border( w_border, c_white, veh->name, c_white );
            wnoutrefresh( w_border );
            draw_info( here );
        } );
    }
    return current_ui;
}

void veh_app_interact::app_loop( map &here )
{
    bool done = false;
    while( !done ) {
        // scope this tighter so that this ui is hidden when app_actions[ret]() triggers
        {
            ui.reset();
            shared_ptr_fast<ui_adaptor> current_ui = create_or_get_ui_adaptor( here );
            ui_manager::redraw();
            shared_ptr_fast<uilist_impl> input_ui = imenu.create_or_get_ui();
            imenu.query();
        }

        int ret = imenu.ret;
        if( ret < 0 || static_cast<size_t>( ret ) >= imenu.entries.size() ) {
            done = true;
        } else if( imenu.entries[ret].enabled ) {
            app_actions[ret]();
        }
        // Player activity queued up, close interaction menu
        if( veh == nullptr || !act.is_null() || !get_player_character().activity.is_null() ) {
            done = true;
        }
    }
}
