#include "game.h"
#include "handle_liquid.h"
#include "inventory.h"
#include "itype.h"
#include "map_iterator.h"
#include "output.h"
#include "overmapbuffer.h"
#include "player_activity.h"
#include "skill.h"
#include "string_input_popup.h"
#include "ui.h"
#include "ui_manager.h"
#include "units.h"
#include "veh_appliance.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_range.h"

static const activity_id ACT_VEHICLE( "ACT_VEHICLE" );

static const itype_id fuel_type_battery( "battery" );

static const quality_id qual_HOSE( "HOSE" );

static const trait_id trait_DEBUG_HS( "DEBUG_HS" );

static const vpart_id vpart_ap_standing_lamp( "ap_standing_lamp" );

static const vproto_id vehicle_prototype_none( "none" );

static const std::string flag_APPLIANCE( "APPLIANCE" );
static const std::string flag_WIRING( "WIRING" );


// Width of the entire set of windows. 60 is sufficient for
// all tested cases while remaining within the 80x24 limit.
// TODO: make this dynamic in the future.
static const int win_width = 60;

vpart_id vpart_appliance_from_item( const itype_id &item_id )
{
    for( const std::pair<const vpart_id, vpart_info> &e : vpart_info::all() ) {
        const vpart_info &vp = e.second;
        if( vp.base_item == item_id && vp.has_flag( flag_APPLIANCE ) ) {
            return vp.get_id();
        }
    }
    debugmsg( "item %s is not base item of any appliance!", item_id.c_str() );
    return vpart_ap_standing_lamp;
}

void place_appliance( const tripoint &p, const vpart_id &vpart, const cata::optional<item> &base )
{
    map &here = get_map();
    vehicle *veh = here.add_vehicle( vehicle_prototype_none, p, 0_degrees, 0, 0 );

    if( !veh ) {
        debugmsg( "error constructing vehicle" );
        return;
    }

    if( base ) {
        item copied = *base;
        veh->install_part( point_zero, vpart, std::move( copied ) );
    } else {
        veh->install_part( point_zero, vpart );
    }
    veh->name = vpart->name();

    veh->add_tag( flag_APPLIANCE );

    // Update the vehicle cache immediately,
    // or the appliance will be invisible for the first couple of turns.
    here.add_vehicle_to_cache( veh );

    // Connect to any neighbouring appliances or wires once
    std::unordered_set<const vehicle *> connected_vehicles;
    for( const tripoint &trip : here.points_in_radius( p, 1 ) ) {
        const optional_vpart_position vp = here.veh_at( trip );
        if( !vp ) {
            continue;
        }
        const vehicle &veh_target = vp->vehicle();
        if( veh_target.has_tag( flag_APPLIANCE ) || veh_target.has_tag( flag_WIRING ) ) {
            if( connected_vehicles.find( &veh_target ) == connected_vehicles.end() ) {
                veh->connect( p, trip );
                connected_vehicles.insert( &veh_target );
            }
        }
    }
}

// uilist_callback whose sole responsibility is to draw the
// connecting borders between the uilist and the info window.
class app_uilist_handler : public uilist_callback
{
        void refresh( uilist *imenu ) override {
            //NOLINTNEXTLINE(cata-use-named-point-constants)
            mvwputch( imenu->window, point( 0, 0 ), c_white, LINE_XXXO );
            mvwputch( imenu->window, point( win_width - 1, 0 ), c_white, LINE_XOXX );
            wnoutrefresh( imenu->window );
        }
};

static app_uilist_handler app_callback;

player_activity veh_app_interact::run( vehicle &veh, const point &p )
{
    veh_app_interact ap( veh, p );
    ap.app_loop();
    return ap.act;
}

// Registers general appliance actions from keybindings
veh_app_interact::veh_app_interact( vehicle &veh, const point &p )
    : a_point( p ), veh( &veh ), ctxt( "APP_INTERACT", keyboard_mode::keycode )
{
    ctxt.register_directions();
    ctxt.register_action( "REFILL" );
    ctxt.register_action( "SIPHON" );
    ctxt.register_action( "RENAME" );
    ctxt.register_action( "REMOVE" );
    ctxt.register_action( "UNPLUG" );
}

void veh_app_interact::init_ui_windows()
{
    int height_info = veh->get_printable_fuel_types().size() + 2;
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
    const int width_input = win_width;
    const int height = height_info + height_input + 2;

    // Center the UI
    point topleft( TERMX / 2 - win_width / 2, TERMY / 2 - height / 2 );
    w_border = catacurses::newwin( height, win_width, topleft );
    //NOLINTNEXTLINE(cata-use-named-point-constants)
    w_info = catacurses::newwin( height_info, width_info, topleft + point( 1, 1 ) );

    // Setup modifications to the uilist to integrate it into the UI
    imenu.w_width_setup = width_input;
    imenu.w_x_setup = topleft.x;
    imenu.w_y_setup = topleft.y + height_info;
    imenu.allow_cancel = true;
    imenu.border_color = c_white;
    imenu.callback = &app_callback;
    imenu.setup();
}

void veh_app_interact::draw_info()
{
    werase( w_info );

    int row = 0;
    // Fuel indicators
    veh->print_fuel_indicators( w_info, point( 0, row ), 0, true, true, true, true );
    row += veh->get_printable_fuel_types().size();

    // Onboard battery power
    if( !veh->batteries.empty() ) {
        std::pair<int, int> battery = veh->battery_power_level();
        nc_color batt_col = c_yellow;
        if( battery.second > 0 ) {
            batt_col = battery.first == 0 ? c_light_red :
                       battery.first == battery.second ? c_light_green : c_yellow;
        }
        mvwprintz( w_info, point( 0, row ), c_white, _( "Onboard battery power: " ) );
        wprintz( w_info, batt_col, string_format( "%d/%d", battery.first, battery.second ) );
        row++;
    }

    auto print_charge = [this]( const std::string & lbl, int rate, int row ) {
        std::string rstr;
        if( std::abs( rate ) > 10000 ) {
            //~ Power in killoWatts. %+4.1f is a 4 digit number with 1 decimal point (ex: 2198.3 kW)
            rstr = string_format( _( "%+4.1f kW" ), rate / 1000.f );
        } else {
            //~ Power in Watts. %+4d is a 4 digit whole number (ex: 4737 W)
            rstr = string_format( _( "%+4d W" ), rate );
        }
        nc_color rcol = rate < 0 ? c_light_red :
                        rate > 0 ? c_light_green : c_yellow;
        mvwprintz( w_info, point( 0, row ), c_white, lbl );
        wprintz( w_info, rcol, rstr );
    };

    // Battery power output
    int charge_rate = veh->net_battery_charge_rate_w();
    print_charge( _( "Battery power output: " ), charge_rate, row );
    row++;

    // Reactor power output
    if( !veh->reactors.empty() ) {
        int rate = veh->active_reactor_epower_w( true );
        print_charge( _( "Reactor power output: " ), rate, row );
        row++;
    }

    // Wind power output
    if( !veh->wind_turbines.empty() ) {
        int rate = veh->total_wind_epower_w();
        print_charge( _( "Wind power output: " ), rate, row );
        row++;
    }

    // Solar power output
    if( !veh->solar_panels.empty() ) {
        int rate = veh->total_solar_epower_w();
        print_charge( _( "Solar power output: " ), rate, row );
        row++;
    }

    // Water power output
    if( !veh->water_wheels.empty() ) {
        int rate = veh->total_water_wheel_epower_w();
        print_charge( _( "Water power output: " ), rate, row );
        row++;
    }

    // Alternator power output
    if( !veh->alternators.empty() ) {
        int rate = veh->total_alternator_epower_w();
        print_charge( _( "Alternator power output: " ), rate, row );
        row++;
    }

    // Other power output
    if( !veh->accessories.empty() ) {
        int rate = veh->total_accessory_epower_w();
        print_charge( _( "Total power consumption: " ), rate, row );
        row++;
    }

    wnoutrefresh( w_info );
}

bool veh_app_interact::can_refill()
{
    for( const vpart_reference &vpr : veh->get_all_parts() ) {
        if( vpr.part().can_reload() ) {
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

bool veh_app_interact::can_unplug()
{
    vehicle_part_range vpr = veh->get_all_parts();
    return std::any_of( vpr.begin(), vpr.end(), []( const vpart_reference & ref ) {
        return ref.vehicle().part_flag( static_cast<int>( ref.part_index() ), "POWER_TRANSFER" );
    } );
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
                units::volume mult = units::legacy_volume_factor / item::find_type(
                                         vpr->ammo_current() )->stack_size;
                double vcur = to_liter( vpr->ammo_remaining() * mult );
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

void veh_app_interact::refill()
{
    std::vector<vehicle_part *> ptlist;
    for( const vpart_reference &vpr : veh->get_all_parts() ) {
        if( vpr.part().can_reload() ) {
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
        act.str_values.push_back( pt->info().get_id().str() );
        const point q = veh->coord_translate( pt->mount );
        map &here = get_map();
        for( const tripoint &p : veh->get_points( true ) ) {
            act.coord_set.insert( here.getabs( p ) );
        }
        act.values.push_back( here.getabs( veh->global_pos3() ).x + q.x );
        act.values.push_back( here.getabs( veh->global_pos3() ).y + q.y );
        act.values.push_back( a_point.x );
        act.values.push_back( a_point.y );
        act.values.push_back( -a_point.x );
        act.values.push_back( -a_point.y );
        act.values.push_back( veh->index_of_part( pt ) );
    }
}

void veh_app_interact::siphon()
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
    const int idx = veh->find_part( base );
    item liquid( base.legacy_front() );
    const int liq_charges = liquid.charges;
    if( liquid_handler::handle_liquid( liquid, nullptr, 1, nullptr, veh, idx ) ) {
        veh->drain( idx, liq_charges - liquid.charges );
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

void veh_app_interact::remove()
{
    int const part = veh->part_at( a_point );
    vehicle_part &vp = veh->part( part >= 0 ? part : 0 );
    const vpart_info &vpinfo = vp.info();
    const requirement_data reqs = vpinfo.removal_requirements();
    Character &you = get_player_character();
    const inventory &inv = you.crafting_inventory();
    std::string msg;
    bool can_remove = reqs.can_make_with_inventory( inv, is_crafting_component );
    if( !can_remove ) {
        msg += _( "Insufficient components/tools!\n" );
        msg += reqs.list_missing();
    }

    int time = vpinfo.removal_time( you );
    if( you.has_trait( trait_DEBUG_HS ) ) {
        can_remove = true;
        time = 1;
    }

    if( !can_remove ) {
        popup( msg );
        //~ Prompt the player if they want to remove the appliance. %s = appliance name.
    } else if( query_yn( _( "Are you sure you want to take down the %s?" ), veh->name ) ) {
        act = player_activity( ACT_VEHICLE, time, static_cast<int>( 'O' ) );
        act.str_values.push_back( vpinfo.get_id().str() );
        const point q = veh->coord_translate( vp.mount );
        map &here = get_map();
        for( const tripoint &p : veh->get_points( true ) ) {
            act.coord_set.insert( here.getabs( p ) );
        }
        act.values.push_back( here.getabs( veh->global_pos3() ).x + q.x );
        act.values.push_back( here.getabs( veh->global_pos3() ).y + q.y );
        act.values.push_back( a_point.x );
        act.values.push_back( a_point.y );
        act.values.push_back( -a_point.x );
        act.values.push_back( -a_point.y );
        act.values.push_back( veh->index_of_part( &vp ) );
    }
}

void veh_app_interact::unplug()
{
    veh->shed_loose_parts();
    int const part = veh->part_at( a_point );
    vehicle_part &vp = veh->part( part >= 0 ? part : 0 );
    act = player_activity( ACT_VEHICLE, 1, static_cast<int>( 'u' ) );
    act.str_values.push_back( vp.info().get_id().str() );
    const point q = veh->coord_translate( vp.mount );
    map &here = get_map();
    for( const tripoint &p : veh->get_points( true ) ) {
        act.coord_set.insert( here.getabs( p ) );
    }
    act.values.push_back( here.getabs( veh->global_pos3() ).x + q.x );
    act.values.push_back( here.getabs( veh->global_pos3() ).y + q.y );
    act.values.push_back( a_point.x );
    act.values.push_back( a_point.y );
    act.values.push_back( -a_point.x );
    act.values.push_back( -a_point.y );
    act.values.push_back( veh->index_of_part( &vp ) );
}

void veh_app_interact::populate_app_actions()
{
    const std::string ctxt_letters = ctxt.get_available_single_char_hotkeys();
    imenu.entries.clear();
    app_actions.clear();

    /******************** General actions ********************/
    // Refill
    app_actions.emplace_back( [this]() {
        refill();
    } );
    imenu.addentry( -1, can_refill(), ctxt.keys_bound_to( "REFILL" ).front(),
                    ctxt.get_action_name( "REFILL" ) );
    // Siphon
    app_actions.emplace_back( [this]() {
        siphon();
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
    app_actions.emplace_back( [this]() {
        remove();
    } );
    imenu.addentry( -1, true, ctxt.keys_bound_to( "REMOVE" ).front(),
                    ctxt.get_action_name( "REMOVE" ) );
    // Unplug
    app_actions.emplace_back( [this]() {
        unplug();
    } );
    imenu.addentry( -1, can_unplug(), ctxt.keys_bound_to( "UNPLUG" ).front(),
                    ctxt.get_action_name( "UNPLUG" ) );

    /*************** Get part-specific actions ***************/
    std::vector<uilist_entry> tmp_opts;
    std::vector<std::function<void()>> tmp_acts;
    veh->set_electronics_menu_options( tmp_opts, tmp_acts );
    for( size_t i = 0; i < tmp_opts.size() && i < ctxt_letters.size(); i++ ) {
        imenu.addentry( -1, tmp_opts[i].enabled, ctxt_letters[i], tmp_opts[i].txt );
        app_actions.emplace_back( tmp_acts[i] );
    }
    imenu.setup();
}

shared_ptr_fast<ui_adaptor> veh_app_interact::create_or_get_ui_adaptor()
{
    shared_ptr_fast<ui_adaptor> current_ui = ui.lock();
    if( !current_ui ) {
        populate_app_actions();
        ui = current_ui = make_shared_fast<ui_adaptor>();
        current_ui->on_screen_resize( [this]( ui_adaptor & cui ) {
            init_ui_windows();
            cui.position_from_window( catacurses::stdscr );
        } );
        current_ui->mark_resize();
        current_ui->on_redraw( [this]( const ui_adaptor & ) {
            draw_border( w_border, c_white, veh->name, c_white );
            wnoutrefresh( w_border );
            draw_info();
        } );
    }
    return current_ui;
}

void veh_app_interact::app_loop()
{
    bool done = false;
    bool repop_actions = false;
    while( !done ) {
        if( repop_actions ) {
            populate_app_actions();
            repop_actions = false;
        }
        shared_ptr_fast<ui_adaptor> current_ui = create_or_get_ui_adaptor();
        ui_manager::redraw();
        shared_ptr_fast<ui_adaptor> input_ui = imenu.create_or_get_ui_adaptor();
        imenu.query();
        int ret = imenu.ret;
        if( ret < 0 || static_cast<size_t>( ret ) >= imenu.entries.size() ) {
            done = true;
        } else if( imenu.entries[ret].enabled ) {
            app_actions[ret]();
            repop_actions = true;
        }
        // Player activity queued up, close interaction menu
        if( !act.is_null() || !get_player_character().activity.is_null() ) {
            done = true;
        }
    }
}
