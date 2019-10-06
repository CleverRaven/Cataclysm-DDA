#include "inventory_transfer.h"
#include "avatar.h"
#include "uistate.h"
#include "bionics.h"
#include "input.h"
#include "output.h"
#include "options.h"
#include "vehicle.h"
#include "messages.h"
#include "string_input_popup.h"
#include "item_search.h"
#include "map.h"

void create_transfer_menu()
{
    inventory_transfer new_inv;
    if( uistate.transfer_save.move_all_is_processing ) {
        new_inv.init();
        new_inv.move_all_items_iteration();
    } else {
        new_inv.display();
        uistate.adv_func = create_transfer_menu;
    }
}

/*******    main    *******/

inventory_transfer::inventory_transfer()
{
    save_state = &uistate.transfer_save;
    reset_on_exit = false;
}

inventory_transfer::~inventory_transfer()
{
    save_settings_internal();
    if( is_full_exit() ) {
        werase( pane_right->window );
    }
}

void inventory_transfer::save_settings()
{
    advanced_inv_base::save_settings();
    save_settings_internal();
}

void inventory_transfer::save_settings_internal()
{
    inventory_transfer_save_state &save = *static_cast<inventory_transfer_save_state *>( save_state );
    save.active_left = cur_pane == pane.get();
    pane_right.get()->save_settings( is_full_exit() && reset_on_exit );
}

void inventory_transfer::init_pane()
{
    int itemsPerPage = w_height - 2 - 5; // 2 for the borders, 5 for the header stuff

    inventory_transfer_save_state &save = *static_cast<inventory_transfer_save_state *>( save_state );

    catacurses::window window = catacurses::newwin( w_height, w_width / 2, point( colstart,
                                headstart + head_height ) );
    pane = std::make_unique< inventory_transfer_pane>( squares, save.pane );

    catacurses::window window_right = catacurses::newwin( w_height, w_width / 2,
                                      point( colstart + w_width / 2, headstart + head_height ) );
    pane_right = std::make_unique< inventory_transfer_pane>( squares, save.pane_right );

    inventory_transfer_pane *p1 = static_cast<inventory_transfer_pane *>( pane.get() );
    inventory_transfer_pane *p2 = static_cast<inventory_transfer_pane *>
                                  ( pane_right.get() );

    p1->init( itemsPerPage, window, p2 );
    p2->init( itemsPerPage, window_right, p1 );

    cur_pane = save.active_left ? pane.get() :
               pane_right.get(); //need this to avoid checking for nullptr in set_active_pane
    set_active_pane( cur_pane );
}

void inventory_transfer::set_active_pane( advanced_inv_pane *p )
{
    static_cast<inventory_transfer_pane *>( cur_pane )->set_active( false );
    cur_pane = p;
    static_cast<inventory_transfer_pane *>( cur_pane )->set_active( true );
}
advanced_inv_pane *inventory_transfer::get_other_pane()
{
    return static_cast<inventory_transfer_pane *>( get_pane() )->get_other_pane();
}

input_context inventory_transfer::register_actions()
{
    input_context ctxt = advanced_inv_base::register_actions();
    ctxt.register_action( "MOVE_SINGLE_ITEM" );
    ctxt.register_action( "MOVE_VARIABLE_ITEM" );
    ctxt.register_action( "MOVE_ITEM_STACK" );
    ctxt.register_action( "MOVE_ALL_ITEMS" );
    ctxt.register_action( "TOGGLE_TAB" );
    ctxt.register_action( "SAVE_DEFAULT" );
    ctxt.register_action( "ITEMS_DEFAULT" );
    return ctxt;
}

std::string inventory_transfer::process_actions( input_context &ctxt )
{
    inventory_transfer_save_state &save = *static_cast<inventory_transfer_save_state *>( save_state );
    const std::string action = advanced_inv_base::process_actions( ctxt );
    if( action == "MOVE_SINGLE_ITEM" ||
        action == "MOVE_VARIABLE_ITEM" ||
        action == "MOVE_ITEM_STACK" ) {
        bool to_vehicle;
        advanced_inv_area *darea = query_destination( to_vehicle );
        if( darea == nullptr ) {
            return action;
        }
        int amount_to_move = query_charges( *darea, to_vehicle, action );
        if( amount_to_move <= 0 ) {
            return action;
        }
        move_item( *darea, to_vehicle, amount_to_move );
    } else if( action == "MOVE_ALL_ITEMS" ) {
        bool to_vehicle;
        advanced_inv_area *darea = query_destination( to_vehicle );
        if( darea == nullptr ) {
            return action;
        }
        exit = setup_move_all( *darea, to_vehicle );
        if( !exit ) {
            refresh( recalc, true );
        }
    } else if( action == "TOGGLE_TAB" ) {
        set_active_pane( get_other_pane() );
        refresh( recalc, true );
    } else if( action == "ITEMS_DEFAULT" ) {
        pane->set_area( squares[save.default_area_left],
                        squares[save.default_area_left].is_vehicle_default() );
        pane_right->set_area( squares[save.default_area_right],
                              squares[save.default_area_right].is_vehicle_default() );
        refresh( true, true );
    } else if( action == "SAVE_DEFAULT" ) {
        save.default_area_left = pane->get_area().info.id;
        save.default_area_right = pane_right->get_area().info.id;
        popup( _( "Default layout was saved." ) );
        refresh( recalc, true );
    }

    return action;
}

advanced_inv_area *inventory_transfer::query_destination( bool &to_vehicle )
{
    using ainfo = advanced_inv_area_info;
    inventory_transfer_save_state &save = *static_cast<inventory_transfer_save_state *>( save_state );

    advanced_inv_pane &dpane = *get_other_pane();
    advanced_inv_area &darea = dpane.get_area();
    to_vehicle = dpane.is_in_vehicle();
    if( darea.info.type != ainfo::AREA_TYPE_MULTI ) {
        if( darea.is_valid() ) {
            return &darea;
        }
        popup( _( "You can't put items there!" ) );
        refresh( recalc, true ); // the popup has messed the screen up.
        return nullptr;
    }

    uilist menu;
    menu.text = _( "Select destination" );
    menu.pad_left = 9; // free space for the squares
    query_destination_callback cb( squares );
    menu.callback = &cb;

    int selected_idx = 0;
    {
        auto &locs = squares[advanced_inv_area_info::AIM_ALL].info.multi_locations;
        int iter = 0;
        for( auto &l : locs ) {
            advanced_inv_area &s = squares[l];
            const int itm_count = s.get_item_count( true );
            const int itm_max = s.get_max_items( true );
            std::string prefix = "";
            if( itm_max > 0 ) {
                prefix = string_format( "%4d/%d", itm_count, itm_max );
                if( itm_count >= itm_max ) {
                    prefix += _( " (FULL)" );
                }
            }
            if( l == save.last_popup_dest ) {
                selected_idx = iter;
            }
            menu.addentry( l,
                           s.is_valid() && s.info.id != get_other_pane()->get_area().info.id,
                           s.info.minimapname[0],
                           prefix + " " + s.info.get_name() + " " + ( s.has_vehicle() ? s.get_name( true ) : "" ) );
            iter++;
        }
    }
    menu.selected = selected_idx;
    menu.show();
    while( menu.ret == UILIST_WAIT_INPUT ) {
        menu.query( false ); // query, but don't loop
    }
    refresh( recalc, true );
    if( menu.ret >= 0 ) {
        assert( squares[menu.ret].is_valid() );
        save.last_popup_dest = menu.ret;
        to_vehicle = squares[menu.ret].has_vehicle();
        return &squares[menu.ret];
    }
    return nullptr;
}

int inventory_transfer::query_charges( const advanced_inv_area &darea, bool to_vehicle,
                                       const std::string &action )
{
    using ainfo = advanced_inv_area_info;
    player &p = g->u;
    int charges;

    advanced_inv_pane &spane = *get_pane();
    // current item in source pane, might be null
    advanced_inv_listitem *sitem = cur_pane->get_cur_item_ptr();
    if( sitem == nullptr ) {
        return -1;
    }

    assert( darea.info.type != ainfo::AIM_ALL ); // should be a specific location instead
    assert( !sitem.items.empty() ); // valid item is obviously required
    const item &it = *sitem->items.front();
    const bool by_charges = it.count_by_charges();

    // default to move all, unless if being equipped
    const int input_amount = by_charges ? it.charges : action == "MOVE_SINGLE_ITEM" ? 1 : sitem->stacks;
    assert( input_amount > 0 ); // there has to be something to begin with
    charges = input_amount;

    // Includes moving from/to inventory and around on the map.
    if( it.made_of_from_type( LIQUID ) && !it.is_frozen_liquid() ) {
        popup( _( "You can't pick up a liquid." ) );
        refresh( recalc, true );
        return -1;
    }

    const units::volume free_volume = darea.free_volume( to_vehicle );
    // Check volume, this should work the same for inventory, map and vehicles, but not for worn
    const int room_for = it.charges_per_volume( free_volume );
    if( charges > room_for && darea.info.id != ainfo::AIM_WORN ) {
        if( room_for <= 0 ) {
            popup( _( "Destination area is full.  Remove some items first." ) );
            refresh( recalc, true );
            return -1;
        }
        charges = std::min( room_for, charges );
    }
    // Map and vehicles have a maximal item count, check that. Inventory does not have this.
    if( darea.info.type != ainfo::AREA_TYPE_PLAYER ) {
        const int cntmax = darea.get_max_items( to_vehicle ) - darea.get_item_count( to_vehicle );
        // For items counted by charges, adding it adds 0 items if something there stacks with it.        bool stacks_with(const item& it) {
        const auto all_items = darea.get_items( to_vehicle );
        const bool can_stack = std::any_of( all_items.begin(), all_items.end(),
        [&it]( const std::list<item *> &istack ) {
            return istack.front()->stacks_with( it );
        } );

        const bool adds0 = by_charges && can_stack;
        if( cntmax <= 0 && !adds0 ) {
            popup( _( "Destination area has too many items.  Remove some first." ) );
            refresh( recalc, true );
            return -1;
        }
        // Items by charge count as a single item, regardless of the charges. As long as the
        // destination can hold another item, one can move all charges.
        if( !by_charges ) {
            charges = std::min( cntmax, charges );
        }
    }
    // Inventory has a weight capacity, map and vehicle don't have that
    if( darea.info.type == ainfo::AREA_TYPE_PLAYER ) {
        const units::mass unitweight = it.weight() / ( by_charges ? it.charges : 1 );
        const units::mass max_weight = p.has_trait( trait_id( "DEBUG_STORAGE" ) ) ?
                                       units::mass_max : p.weight_capacity() * 4 - p.weight_carried();
        if( unitweight > 0_gram && unitweight * charges > max_weight ) {
            const int weightmax = max_weight / unitweight;
            if( weightmax <= 0 ) {
                popup( _( "This is too heavy!" ) );
                refresh( recalc, true );
                return -1;
            }
            charges = std::min( weightmax, charges );
        }
    }
    // handle how many of armor type we can equip (max of 2 per type)
    if( darea.info.id == ainfo::AIM_WORN ) {
        const auto &id = it.typeId();
        // how many slots are available for the item?
        const int slots_available = MAX_WORN_PER_TYPE - p.amount_worn( id );
        // base the amount to equip on amount of slots available
        charges = std::min( slots_available, input_amount );
    }
    // Now we have the final amount. Query if requested or limited room left.
    if( action == "MOVE_VARIABLE_ITEM" || charges < input_amount ) {
        const int count = by_charges ? it.charges : sitem->stacks;
        const char *msg = nullptr;
        std::string popupmsg;
        if( charges >= input_amount ) {
            msg = _( "How many do you want to move? [Have %d] (0 to cancel)" );
            popupmsg = string_format( msg, count );
        } else {
            msg = _( "Destination can only hold %d! Move how many? [Have %d] (0 to cancel)" );
            popupmsg = string_format( msg, charges, count );
        }
        // At this point amount contains the maximal amount that the destination can hold.
        const int possible_max = std::min( input_amount, charges );
        if( charges <= 0 ) {
            popup( _( "The destination is already full!" ) );
        } else {
            charges = string_input_popup()
                      .title( popupmsg )
                      .width( 20 )
                      .only_digits( true )
                      .query_int();
        }
        if( charges <= 0 ) {
            refresh( recalc, true );
            return -1;
        }
        if( charges > possible_max ) {
            charges = possible_max;
        }
    }
    return charges;
}

void inventory_transfer::move_item( advanced_inv_area &move_to, bool to_vehicle,
                                    int amount_to_move )
{
    assert( amount_to_move > 0 );

    using ainfo = advanced_inv_area_info;
    player &p = g->u;

    advanced_inv_pane &spane = *get_pane();
    // current item in source pane, might be null
    advanced_inv_listitem *sitem = spane.get_cur_item_ptr();

    if( sitem == nullptr || !sitem->is_item_entry() ) {
        return;
    }

    advanced_inv_area &sarea = sitem->get_area();

    if( sarea.is_same( move_to ) && sitem->from_vehicle == to_vehicle ) {
        popup( _( "Source area is the same as destination (%s)." ), move_to.get_name( to_vehicle ) );
        refresh( recalc, true ); // popup has messed up the screen
        return;
    }
    assert( !sitem->items.empty() );
    const bool by_charges = sitem->items.front()->count_by_charges();
    // This makes sure that all item references in the advanced_inventory_pane::items vector
    // are recalculated, even when they might not have changed, but they could (e.g. items
    // taken from inventory, but unable to put into the cargo trunk go back into the inventory,
    // but are potentially at a different place).
    refresh( true, redraw );

    if( sarea.info.id == ainfo::AIM_INVENTORY && move_to.info.id == ainfo::AIM_WORN ) {
        do_return_entry();

        p.assign_activity( activity_id( "ACT_WEAR" ) );
        p.activity.targets.emplace_back( g->u, sitem->items.front() );
        p.activity.values.push_back( amount_to_move );

        exit = true;
    } else if( sarea.info.type == ainfo::AREA_TYPE_PLAYER ) {
        do_return_entry();

        // if worn, we need to fix with the worn index number (starts at -2, as -1 is weapon)
        int idx = sarea.info.id == ainfo::AIM_INVENTORY ? sitem->idx : player::worn_position_to_index(
                      sitem->idx );

        if( sarea.info.id == ainfo::AIM_WORN && move_to.info.id == ainfo::AIM_INVENTORY ) {
            // this is ok because worn items are never stacked (can't move more than 1).
            p.takeoff( idx );

            exit = true;
        } else {
            // important if item is worn
            if( p.can_unwield( p.i_at( idx ) ).success() ) {
                p.assign_activity( activity_id( "ACT_DROP" ) );
                p.activity.placement = move_to.offset;
                if( !to_vehicle ) {
                    p.activity.str_values.push_back( "force_ground" );
                }
                p.activity.values.push_back( idx );
                p.activity.values.push_back( amount_to_move );

                exit = true;
            }
        }
    } else { // from map/vehicle
        do_return_entry();

        if( move_to.info.id == ainfo::AIM_INVENTORY ) {
            p.assign_activity( activity_id( "ACT_PICKUP" ) );
        } else if( move_to.info.id == ainfo::AIM_WORN ) {
            p.assign_activity( activity_id( "ACT_WEAR" ) );
        } else { // Vehicle and map destinations are handled similarly.
            p.assign_activity( activity_id( "ACT_MOVE_ITEMS" ) );
            p.activity.values.push_back( to_vehicle );
            p.activity.coords.push_back( move_to.offset );
        }

        tripoint target = p.pos() + sarea.offset;
        if( by_charges ) {

            p.activity.targets.push_back( sarea.generate_item_location( sitem->from_vehicle,
                                          sitem->items.front() ) );
            p.activity.values.push_back( amount_to_move );
        } else {
            for( std::list<item *>::iterator it = sitem->items.begin(); amount_to_move > 0 &&
                 it != sitem->items.end(); ++it ) {
                p.activity.targets.push_back( sarea.generate_item_location( sitem->from_vehicle, *it ) );
                p.activity.values.push_back( 0 );
                --amount_to_move;
            }
        }

        exit = true;
    }
}

bool inventory_transfer::setup_move_all( advanced_inv_area &move_to, bool to_vehicle )
{
    inventory_transfer_save_state &save = *static_cast<inventory_transfer_save_state *>( save_state );
    assert( !save.adv_inv_move_all_is_processing ); // shouldn't be here again, before finishing previous move all
    using ainfo = advanced_inv_area_info;
    player &p = g->u;

    inventory_transfer_pane &spane = *static_cast<inventory_transfer_pane *>( get_pane() );
    advanced_inv_area &sarea = spane.get_area();

    if( !spane.has_items() ) {
        popup( _( "There are no items to be moved!" ) );
        return false;
    }

    if( !spane.can_transef_all() &&
        !query_yn( _( "There isn't enough room, do you really want to move all?" ) ) ) {
        return false;
    }

    if( sarea.info.type == ainfo::AREA_TYPE_MULTI ) {
        //do nothing
    } else if( sarea.is_same( move_to ) && spane.is_in_vehicle() == to_vehicle ) {
        popup( _( "Source area is the same as destination (%s)." ), move_to.get_name( to_vehicle ) );
        return false;
    }

    //Set up moving iterator
    //We need to switch activity to ACT_MOVE_ITEMS (or similar) and then back multiple times, this will keep track of what to move next
    save.move_all_is_processing = true;
    save.move_all_idx = 0;
    save.move_all_locations.clear();
    for( ainfo::aim_location loc : sarea.info.multi_locations ) {
        const advanced_inv_area &a = squares[loc];
        if( a.is_valid() ) {
            if( a.has_vehicle() && a.get_item_count( true ) > 0 ) {
                save.move_all_locations.emplace_back( loc, true );
            }
            if( a.get_item_count( false ) > 0 ) {
                save.move_all_locations.emplace_back( loc, false );
            }
        }
    }
    save.move_all_filter = spane.get_filter_func();
    save.move_all_to = std::make_pair( move_to.info.id, to_vehicle );
    move_all_items_iteration();

    return true;
}
void inventory_transfer::move_all_items_iteration()
{
    inventory_transfer_save_state &save = *static_cast<inventory_transfer_save_state *>( save_state );

    auto move_from_data = save.move_all_locations[save.move_all_idx];
    advanced_inv_area &move_from = squares[move_from_data.first];
    bool from_vehicle = move_from_data.second;

    advanced_inv_area &move_to = squares[save.move_all_to.first];
    bool to_vehicle = save.move_all_to.second;

    move_all_items( move_from, from_vehicle, move_to, to_vehicle, save.move_all_filter );

    ++save.move_all_idx;
    if( save.move_all_idx >= save.move_all_locations.size() ) {
        save.move_all_is_processing = false;
    }
}

bool inventory_transfer::move_all_items( advanced_inv_area &move_from, bool from_vehicle,
        advanced_inv_area &move_to, bool to_vehicle, std::function<bool( const item & )> filter )
{
    using ainfo = advanced_inv_area_info;
    player &p = g->u;

    if( !get_option<bool>( "CLOSE_ADV_INV" ) ) {
        // Why is this here? It's because the activity backlog can act
        // like a stack instead of a single deferred activity in order to
        // accomplish some UI shenanigans. The inventory menu activity is
        // added, then an activity to drop is pushed on the stack, then
        // the drop activity is repeatedly popped and pushed on the stack
        // until all its items are processed. When the drop activity runs out,
        // the inventory menu activity is there waiting and seamlessly returns
        // the player to the menu. If the activity is interrupted instead of
        // completing, both activities are canceled.
        // Thanks to kevingranade for the explanation.
        do_return_entry();
    }

    if( move_from.info.id == ainfo::AIM_INVENTORY || move_from.info.id == ainfo::AIM_WORN ) {
        std::list<std::pair<int, int>> dropped;
        // keep a list of favorites separated, only drop non-fav first if they exist
        std::list<std::pair<int, int>> dropped_favorite;

        if( move_from.info.id == ainfo::AIM_INVENTORY ) {
            for( size_t index = 0; index < p.inv.size(); ++index ) {
                const auto &stack = p.inv.const_stack( index );
                const auto &it = stack.front();

                if( filter( it ) ) {
                    ( it.is_favorite ? dropped_favorite : dropped ).emplace_back( static_cast<int>( index ),
                            it.count_by_charges() ? static_cast<int>( it.charges ) : static_cast<int>( stack.size() ) );
                }
            }
        } else if( move_from.info.id == ainfo::AIM_WORN ) {
            // do this in reverse, to account for vector item removal messing with future indices
            auto iter = p.worn.rbegin();
            for( size_t idx = 0; idx < p.worn.size(); ++idx, ++iter ) {
                const size_t index = p.worn.size() - idx - 1;
                const auto &it = *iter;

                if( filter( it ) ) {
                    ( it.is_favorite ? dropped_favorite : dropped ).emplace_back( player::worn_position_to_index(
                                index ),
                            it.count() );
                }
            }
        }
        if( dropped.empty() ) {
            if( !query_yn( _( "Really drop all your favorite items?" ) ) ) {
                return false;
            }
            dropped = dropped_favorite;
        }

        p.drop( dropped, p.pos() + move_to.offset );
    } else {
        if( move_to.info.id == ainfo::AIM_INVENTORY ) {
            p.assign_activity( activity_id( "ACT_PICKUP" ) );
        } else if( move_to.info.id == ainfo::AIM_WORN ) {
            p.assign_activity( activity_id( "ACT_WEAR" ) );
        } else { // Vehicle and map destinations are handled the same.
            p.assign_activity( activity_id( "ACT_MOVE_ITEMS" ) );
            p.activity.values.push_back( to_vehicle );
            p.activity.coords.push_back( move_to.offset );
        }

        item_stack::iterator stack_begin, stack_end;
        move_from.get_item_iterators( from_vehicle, stack_begin, stack_end );

        // If moving to inventory, worn, or vehicle, silently filter buckets
        // Moving them would cause tons of annoying prompts or spills
        bool filter_buckets = move_to.info.type == ainfo::AREA_TYPE_PLAYER || to_vehicle;
        bool filtered_any_bucket = false;
        tripoint target = p.pos() + move_from.offset;
        // Push item_locations and item counts for all items at placement
        for( item_stack::iterator it = stack_begin; it != stack_end; ++it ) {
            if( !filter( *it ) ) {
                continue;
            }
            if( filter_buckets && it->is_bucket_nonempty() ) {
                filtered_any_bucket = true;
                continue;
            }
            p.activity.targets.push_back( move_from.generate_item_location( from_vehicle, &*it ) );
            // quantity of 0 means move all
            p.activity.values.push_back( 0 );
        }

        if( filtered_any_bucket ) {
            add_msg( m_info, _( "Skipping filled buckets to avoid spilling their contents." ) );
        }
    }
    return true;
}

void inventory_transfer::redraw_pane()
{
    pane->redraw();
    pane_right->redraw();
}
void inventory_transfer::redraw_minimap()
{
    werase( mm_border );
    draw_border( mm_border );
    // minor addition to border for AIM_ALL, sorta hacky
    if( get_pane()->get_area().info.type == advanced_inv_area_info::AREA_TYPE_MULTI ) {
        mvwprintz( mm_border, point_east, c_cyan, utf8_truncate( _( "Mul" ), minimap_width ) );
    }
    if( get_other_pane()->get_area().info.type == advanced_inv_area_info::AREA_TYPE_MULTI ) {
        mvwprintz( mm_border, point_east, c_light_gray, utf8_truncate( _( "Mul" ), minimap_width ) );
    }
    wrefresh( mm_border );

    werase( minimap );
    // get the center of the window
    tripoint pc = { getmaxx( minimap ) / 2, getmaxy( minimap ) / 2, 0 };
    // draw the 3x3 tiles centered around player
    g->m.draw( minimap, g->u.pos() );

    char sym;
    //draw other pane first
    sym = minimap_get_sym( get_other_pane() );

    if( sym != '\0' ) {
        auto a = get_other_pane()->get_area();
        auto pt = pc + a.offset;
        mvwputch( minimap, pt.xy(), c_light_gray.blink(), sym );
    }
    //if both panes are at the same location we only want current one
    sym = minimap_get_sym( get_pane() );
    if( sym != '\0' ) {
        auto a = get_pane()->get_area();
        auto pt = pc + a.offset;
        mvwputch( minimap, pt.xy(), invert_color( c_light_cyan ), sym );
    }

    //bool player_selected = get_pane()->get_area().info.type == advanced_inv_area_info::AREA_TYPE_PLAYER;
    g->u.draw( minimap, g->u.pos(), false );
    wrefresh( minimap );
}

void inventory_transfer::refresh( bool needs_recalc, bool needs_redraw )
{
    advanced_inv_base::refresh( needs_recalc, needs_redraw );

    pane_right->needs_recalc = needs_recalc;
    pane_right->needs_redraw = needs_redraw;
}

/*******    pane    *******/

void inventory_transfer_pane::init( int items_per_page, catacurses::window w,
                                    advanced_inv_pane *other_pane )
{
    columns = { COLUMN_SORTBY_CATEGORY, COLUMN_SORTBY_AMMO, COLUMN_SORTBY_AMMO_TYPE, COLUMN_SORTBY_EXPIRES };
    default_sortby = COLUMN_NAME;

    advanced_inv_pane::init( items_per_page, w );
    other = other_pane;
}

advanced_inv_listitem *inventory_transfer_pane::create_listitem( std::list<item *> list,
        int index, advanced_inv_area &area, bool from_vehicle )
{
    return new inventory_transfer_listitem( list, index, area, from_vehicle );
}
advanced_inv_listitem *inventory_transfer_pane::create_listitem( const item_category *cat )
{
    return new inventory_transfer_listitem( cat );
}

void inventory_transfer_pane::set_area( advanced_inv_area &square, bool show_vehicle )
{
    //nullptr happens during initialization
    if( other != nullptr && square.is_same( other->get_area() ) &&
        show_vehicle == other->is_in_vehicle() ) {
        if( square.has_vehicle() ) {
            cur_area = &square;
            viewing_cargo = other->is_in_vehicle();
            other->advanced_inv_pane::set_area( square, !other->is_in_vehicle() );
        } else {
            other->advanced_inv_pane::set_area( *cur_area, viewing_cargo );
            cur_area = &square;
            viewing_cargo = false;
        }
    } else {
        advanced_inv_pane::set_area( square, show_vehicle );
    }
}

void inventory_transfer_pane::add_items_from_area( advanced_inv_area &area, bool from_cargo,
        units::volume &ret_volume,
        units::mass &ret_weight )
{
    if( area.is_same( other->get_area() ) && from_cargo == other->is_in_vehicle() ) {
        return; //skip if same
    } else {
        advanced_inv_pane::add_items_from_area( area, from_cargo, ret_volume, ret_weight );
    }
}

void inventory_transfer_pane::redraw()
{
    // don't update ui if processing demands
    if( needs_recalc ) {
        recalc();
    } else if( !needs_redraw ) {
        return;
    }
    needs_redraw = false;
    fix_index();

    auto w = window;

    werase( w );

    print_items();

    auto itm = get_cur_item_ptr();
    int width = print_header( itm != nullptr ? itm->get_area() : get_area() );

    bool car = cur_area->has_vehicle() && viewing_cargo;
    auto name = utf8_truncate( cur_area->get_name( viewing_cargo ), width );
    auto desc = utf8_truncate( cur_area->desc[car], width );
    width -= 2 + 1; // starts at offset 2, plus space between the header and the text
    mvwprintz( w, point( 2, 1 ), is_active ? c_green : c_light_gray, name );
    mvwprintz( w, point( 2, 2 ), is_active ? c_light_blue : c_dark_gray, desc );
    trim_and_print( w, point( 2, 3 ), width, is_active ? c_cyan : c_dark_gray, cur_area->flags );

    for( auto &i : additional_info ) {
        print_colored_text( window, i.position, i.color, i.color, i.message );
    }
    const int max_page = ( items.size() + itemsPerPage - 1 ) / itemsPerPage;
    if( is_active && max_page > 1 ) {
        const int page = index / itemsPerPage;
        mvwprintz( w, point( 2, 4 ), c_light_blue, _( "[<] page %d of %d [>]" ), page + 1, max_page );
    }
    if( is_active ) {
        wattron( w, c_cyan );
    }
    draw_border( w, is_active ? c_red : c_dark_gray );
    mvwprintw( w, point( 3, 0 ), _( "< [s]ort: %s >" ),
               advanced_inv_listitem::get_col_data( sortby ).get_sort_name() );
    int max = get_area().get_max_items( viewing_cargo );
    if( max > 0 ) {
        int itemcount = get_area().get_item_count( viewing_cargo );
        const std::string msg = string_format( _( "< %d/%d >" ), itemcount, max );
        mvwprintz( w, point( getmaxx( w ) - utf8_width( msg ) - 1, 0 ), c_dark_gray, msg );
    }

    if( !title.empty() ) {
        std::string title_string = string_format( "<< %s >>", title );
        mvwprintz( w, point( ( getmaxx( w ) - title_string.size() ) / 2, 0 ), c_light_cyan, title_string );
    }

    const char *fprefix = _( "[F]ilter" );
    const char *fsuffix = _( "[R]eset" );

    if( !filter_edit ) {
        std::string s = filter.empty() ? "< %s >" : "< %s: %s >";
        mvwprintw( w, point( 2, getmaxy( w ) - 1 ), string_format( s, fprefix, filter ) );
    }
    if( is_active ) {
        wattroff( w, c_white );
    }

    if( !filter_edit && !filter.empty() ) {
        mvwprintz( w, point( 6 + utf8_width( fprefix ), getmaxy( w ) - 1 ), c_white,
                   filter );
        mvwprintz( w, point( getmaxx( w ) - utf8_width( fsuffix ) - 2, getmaxy( w ) - 1 ), c_white, "%s",
                   fsuffix );
    }
    wrefresh( w );
}

advanced_inv_select_state inventory_transfer_pane::item_selected_state( bool selected )
{
    if( !is_active ) {
        return SELECTSTATE_INACTIVE;
    } else {
        return advanced_inv_pane::item_selected_state( selected );
    }
}


std::function<bool( const item & )> inventory_transfer_pane::get_filter_func()
{
    if( filter.empty() ) {
        return []( const item & ) {
            return true;
        };
    }
    return item_filter_from_string( filter );
}

/*******    listitem    *******/

inventory_transfer_listitem::inventory_transfer_listitem( const item_category *cat ) :
    advanced_inv_listitem( cat )
{
}

inventory_transfer_listitem::inventory_transfer_listitem( const std::list<item *> &list, int index,
        advanced_inv_area &area, bool from_vehicle ) : advanced_inv_listitem( list, index, area,
                    from_vehicle )
{
    init( *list.front() );
}

void inventory_transfer_listitem::init( const item &it )
{
    cond.reserve( 2 );

    cond_size = 2;
    if( it.is_favorite ) {
        cond.emplace_back( it.is_favorite ? '*' : ' ', c_white );
    }
    if( autopickup ) {
        cond.emplace_back( '>', c_white_magenta );
    }
}

void inventory_transfer_listitem::set_print_color( nc_color &retval, nc_color default_col )
{
    if( is_selected == SELECTSTATE_INACTIVE ) {
        retval = c_dark_gray;
    } else {
        advanced_inv_listitem::set_print_color( retval, default_col );
    }

}

std::function<bool( const advanced_inv_listitem *d1, const advanced_inv_listitem *d2, bool &retval )>
inventory_transfer_listitem::compare_function( advanced_inv_columns sortby )
{
    auto f = advanced_inv_listitem::compare_function( sortby );
    if( f != nullptr ) {
        return f;
    }

    using itm = advanced_inv_listitem;
    switch( sortby ) {
        case COLUMN_SORTBY_AMMO:
            return []( const itm * d1, const itm * d2, bool & retval ) {
                retval = d1->items.front()->charges > d2->items.front()->charges;
                return d1->items.front()->charges != d2->items.front()->charges;
            };
        case COLUMN_SORTBY_AMMO_TYPE:
            return []( const itm * d1, const itm * d2, bool & retval ) {

                const std::string a1 = d1->items.front()->ammo_sort_name();
                const std::string a2 = d2->items.front()->ammo_sort_name();
                // There are many items with "false" ammo types (e.g.
                // scrap metal has "components") that actually is not
                // used as ammo, so we consider them as non-ammo.
                const bool ammoish1 = !a1.empty() && a1 != "components" && a1 != "none" && a1 != "NULL";
                const bool ammoish2 = !a2.empty() && a2 != "components" && a2 != "none" && a2 != "NULL";
                if( ammoish1 != ammoish2 ) {
                    retval = ammoish1;
                    return true;
                } else if( ammoish1 && ammoish2 ) {
                    if( a1 == a2 ) {
                        // For items with the same ammo type, we sort:
                        // guns > tools > magazines > ammunition
                        if( d1->items.front()->is_gun() && !d2->items.front()->is_gun() ) {
                            retval = true;
                            return true;
                        }
                        if( !d1->items.front()->is_gun() && d2->items.front()->is_gun() ) {
                            retval = false;
                            return true;
                        }
                        if( d1->items.front()->is_tool() && !d2->items.front()->is_tool() ) {
                            retval = true;
                            return true;
                        }
                        if( !d1->items.front()->is_tool() && d2->items.front()->is_tool() ) {
                            retval = false;
                            return true;
                        }
                        if( d1->items.front()->is_magazine() && d2->items.front()->is_ammo() ) {
                            retval = true;
                            return true;
                        }
                        if( d2->items.front()->is_magazine() && d1->items.front()->is_ammo() ) {
                            retval = false;
                            return true;
                        }
                    }
                    retval = std::lexicographical_compare( a1.begin(), a1.end(),
                                                           a2.begin(), a2.end(),
                                                           sort_case_insensitive_less() );
                    return a1 != a2;
                }
                retval = false;
                return false;
            };
        case COLUMN_SORTBY_EXPIRES:
            return []( const itm * d1, const itm * d2, bool & retval ) {
                retval = d1->items.front()->spoilage_sort_order() < d2->items.front()->spoilage_sort_order();
                return d1->items.front()->spoilage_sort_order() != d2->items.front()->spoilage_sort_order();
            };
            break;
        default:
            return nullptr;
    }
}

/*******    extra    *******/

void query_destination_callback::draw_squares( const uilist *menu )
{
    const int ofs = -25 - 4;
    const std::vector<uilist_entry> &entries = menu->entries;
    for( const uilist_entry &e : entries ) {
        advanced_inv_area square = _squares[e.retval];
        // always show storage option for vehicle storage, if applicable
        const char *bracket = square.has_vehicle() ? "<>" : "[]";
        bool canputitems = e.enabled && square.is_valid();
        //use hotkey as id
        nc_color bcolor = canputitems ? entries[menu->selected].hotkey == e.hotkey ? h_white :
                          c_light_gray : c_red;
        nc_color kcolor = canputitems ? entries[menu->selected].hotkey == e.hotkey ? h_white :
                          c_light_gray : c_red;
        const int x = square.info.hscreen.x + ofs;
        const int y = square.info.hscreen.y + 5;
        mvwprintz( menu->window, point( x, y ), bcolor, "%c", bracket[0] );
        wprintz( menu->window, kcolor, square.info.minimapname );
        wprintz( menu->window, bcolor, "%c", bracket[1] );
    }
}

/**
 * Cancels ongoing move all action.
 * TODO: Make this not needed.
 */
void cancel_aim_processing()
{
    uistate.transfer_save.move_all_is_processing = false;
}
