#include "activity_actor_definitions.h"
#include "activity_handlers.h"
#include "catacharset.h"
#include "character.h"
#include "flag.h"
#include "inventory.h"
#include "iuse_actor.h"
#include "map.h"
#include "options.h"
#include "vehicle.h"
#include "vpart_position.h"

void Character::handle_contents_changed( const std::vector<item_location> &containers )
{
    if( containers.empty() ) {
        return;
    }

    class item_loc_with_depth
    {
        public:
            // NOLINTNEXTLINE(google-explicit-constructor)
            item_loc_with_depth( const item_location &_loc )
                : _loc( _loc ) {
                item_location ancestor = _loc;
                while( ancestor.has_parent() ) {
                    ++_depth;
                    ancestor = ancestor.parent_item();
                }
            }

            const item_location &loc() const {
                return _loc;
            }

            int depth() const {
                return _depth;
            }

        private:
            item_location _loc;
            int _depth = 0;
    };

    class sort_by_depth
    {
        public:
            bool operator()( const item_loc_with_depth &lhs, const item_loc_with_depth &rhs ) const {
                return lhs.depth() < rhs.depth();
            }
    };

    std::multiset<item_loc_with_depth, sort_by_depth> sorted_containers(
        containers.begin(), containers.end() );
    map &m = get_map();

    // unseal and handle containers, from innermost (max depth) to outermost (min depth)
    // so inner containers are always handled before outer containers are possibly removed.
    while( !sorted_containers.empty() ) {
        item_location loc = std::prev( sorted_containers.end() )->loc();
        sorted_containers.erase( std::prev( sorted_containers.end() ) );
        if( !loc ) {
            debugmsg( "invalid item location" );
            continue;
        }
        loc->on_contents_changed();
        const bool handle_drop = loc.where() != item_location::type::map && !is_wielding( *loc );
        bool drop_unhandled = false;
        for( item_pocket *const pocket : loc->get_all_contained_pockets() ) {
            if( pocket && !pocket->sealed() ) {
                // pockets are unsealed but on_contents_changed is not called
                // in contents_change_handler::unseal_pocket_containing
                pocket->on_contents_changed();
            }
            if( pocket && handle_drop && pocket->will_spill() ) {
                // the pocket's contents (with a larger depth value) are not
                // inside `sorted_containers` and can be safely disposed of.
                pocket->handle_liquid_or_spill( *this, /*avoid=*/&*loc );
                // drop the container instead if canceled.
                if( !pocket->empty() ) {
                    // drop later since we still need to access the container item
                    drop_unhandled = true;
                    // canceling one pocket cancels spilling for the whole container
                    break;
                }
            }
        }

        if( loc.has_parent() ) {
            item_location parent_loc = loc.parent_item();
            item_loc_with_depth parent( parent_loc );
            item_pocket *const pocket = loc.parent_pocket();
            pocket->unseal();
            bool exists = false;
            auto it = sorted_containers.lower_bound( parent );
            for( ; it != sorted_containers.end() && it->depth() == parent.depth(); ++it ) {
                if( it->loc() == parent_loc ) {
                    exists = true;
                    break;
                }
            }
            if( !exists ) {
                sorted_containers.emplace_hint( it, parent );
            }
        }

        if( drop_unhandled ) {
            // We can drop the unhandled container now since the container and
            // its contents (with a larger depth) are not inside `sorted_containers`.
            add_msg_player_or_npc(
                _( "To avoid spilling its contents, you set your %1$s on the %2$s." ),
                _( "To avoid spilling its contents, <npcname> sets their %1$s on the %2$s." ),
                loc->display_name(), m.name( pos() )
            );
            item it_copy( *loc );
            loc.remove_item();
            // target item of `loc` is invalidated and should not be used from now on
            m.add_item_or_charges( pos(), it_copy );
        }
    }
}

int Character::count_softwares( const itype_id &id )
{
    int count = 0;
    for( const item_location &it_loc : all_items_loc() ) {
        if( it_loc->is_software_storage() ) {
            for( const item *soft : it_loc->softwares() ) {
                if( soft->typeId() == id ) {
                    count++;
                }
            }
        }
    }
    return count;
}

units::length Character::max_single_item_length() const
{
    return std::max( weapon.max_containable_length(), worn.max_single_item_length() );
}

units::volume Character::max_single_item_volume() const
{
    return std::max( weapon.max_containable_volume(), worn.max_single_item_volume() );
}

std::pair<item_location, item_pocket *> Character::best_pocket( const item &it, const item *avoid,
        bool ignore_settings )
{
    item_location weapon_loc( *this, &weapon );
    std::pair<item_location, item_pocket *> ret = std::make_pair( item_location(), nullptr );
    if( &weapon != &it && &weapon != avoid ) {
        ret = weapon.best_pocket( it, weapon_loc, avoid, false, ignore_settings );
    }
    worn.best_pocket( *this, it, avoid, ret, ignore_settings );
    return ret;
}

item_location Character::try_add( item it, const item *avoid, const item *original_inventory_item,
                                  const bool allow_wield, bool ignore_pkt_settings )
{
    invalidate_inventory_validity_cache();
    invalidate_leak_level_cache();
    itype_id item_type_id = it.typeId();
    last_item = item_type_id;

    // if there's a desired invlet for this item type, try to use it
    bool keep_invlet = false;
    const invlets_bitset cur_inv = allocated_invlets();
    for( const auto &iter : inv->assigned_invlet ) {
        if( iter.second == item_type_id && !cur_inv[iter.first] ) {
            it.invlet = iter.first;
            keep_invlet = true;
            break;
        }
    }
    std::pair<item_location, item_pocket *> pocket = best_pocket( it, avoid, ignore_pkt_settings );
    item_location ret = item_location::nowhere;
    if( pocket.second == nullptr ) {
        if( !has_weapon() && allow_wield && wield( it ) ) {
            ret = item_location( *this, &weapon );
        } else {
            return ret;
        }
    } else {
        // this will set ret to either it, or to stack where it was placed
        item *newit = nullptr;
        pocket.second->add( it, &newit );
        if( !keep_invlet && ( !it.count_by_charges() || it.charges == newit->charges ) ) {
            inv->update_invlet( *newit, true, original_inventory_item );
        }
        pocket.first.on_contents_changed();
        pocket.second->on_contents_changed();
        ret = item_location( pocket.first, newit );
    }

    if( keep_invlet ) {
        ret->invlet = it.invlet;
    }
    ret->on_pickup( *this );
    cached_info.erase( "reloadables" );
    return ret;
}

item_location Character::try_add( item it, int &copies_remaining, const item *avoid,
                                  const item *original_inventory_item,
                                  const bool allow_wield, bool ignore_pkt_settings )
{
    invalidate_inventory_validity_cache();
    invalidate_leak_level_cache();
    itype_id item_type_id = it.typeId();
    last_item = item_type_id;

    // if there's a desired invlet for this item type, try to use it
    char invlet = 0;
    const invlets_bitset cur_inv = allocated_invlets();
    for( const auto &iter : inv->assigned_invlet ) {
        if( iter.second == item_type_id && !cur_inv[iter.first] ) {
            invlet = iter.first;
            break;
        }
    }

    //item copy for test can contain
    item temp_it = item( it );
    temp_it.charges = 1;

    item_location first_item_added;

    std::pair<item_location, item_pocket *> pocket;
    while( copies_remaining > 0 ) {
        pocket = best_pocket( it, avoid, ignore_pkt_settings );
        if( pocket.second == nullptr ) {
            break;
        }

        int max_copies;
        if( !temp_it.is_null() ) {
            const int pocket_max_copies = pocket.second->remaining_capacity_for_item( temp_it );
            const int parent_max_copies = !pocket.first.has_parent() ? INT_MAX :
                                          !pocket.first.parents_can_contain_recursive( &temp_it ).success() ? 0 :
                                          pocket.first.max_charges_by_parent_recursive( temp_it ).value();
            max_copies = std::min( { copies_remaining, pocket_max_copies, parent_max_copies } );
        } else {
            max_copies = copies_remaining;
        }

        std::vector<item *>newits;
        pocket.second->add( it, max_copies, newits );

        // Give invlet to the first item created.
        if( !first_item_added ) {
            first_item_added = item_location( pocket.first, newits.front() );
            if( invlet ) {
                first_item_added->invlet = invlet;
            }
        }
        if( !invlet ) {
            inv->update_invlet( *newits.front(), true, original_inventory_item );
        }

        copies_remaining -= max_copies;

        for( item *it : newits ) {
            it->on_pickup( *this );
        }
        pocket.first.on_contents_changed();
        pocket.second->on_contents_changed();
    }

    if( copies_remaining > 0 && allow_wield && !has_weapon() && wield( it ) ) {
        copies_remaining--;
        if( !first_item_added ) {
            first_item_added = item_location( *this, &weapon );
        }
    }
    if( first_item_added ) {
        cached_info.erase( "reloadables" );
    }
    return first_item_added;
}

item_location Character::i_add( item it, bool /* should_stack */, const item *avoid,
                                const item *original_inventory_item, const bool allow_drop,
                                const bool allow_wield, bool ignore_pkt_settings )
{
    invalidate_inventory_validity_cache();
    invalidate_leak_level_cache();
    item_location added = try_add( it, avoid, original_inventory_item, allow_wield,
                                   ignore_pkt_settings );
    if( added == item_location::nowhere ) {
        if( !allow_wield || !wield( it ) ) {
            if( allow_drop ) {
                return item_location( map_cursor( pos() ), &get_map().add_item_or_charges( pos(), it ) );
            } else {
                return added;
            }
        } else {
            return item_location( *this, &weapon );
        }
    } else {
        return added;
    }
}

item_location Character::i_add( item it, int &copies_remaining,
                                bool /* should_stack */, const item *avoid,
                                const item *original_inventory_item, const bool allow_drop,
                                const bool allow_wield, bool ignore_pkt_settings )
{
    if( it.count_by_charges() ) {
        it.charges = copies_remaining;
        copies_remaining = 0;
        return i_add( it, true, avoid, original_inventory_item,
                      allow_drop, allow_wield, ignore_pkt_settings );
    }
    invalidate_inventory_validity_cache();
    invalidate_leak_level_cache();
    item_location added = try_add( it, copies_remaining, avoid, original_inventory_item, allow_wield,
                                   ignore_pkt_settings );
    if( copies_remaining > 0 ) {
        if( allow_wield && wield( it ) ) {
            copies_remaining--;
            added = added ? added : item_location( *this, &weapon );
        }
        if( allow_drop && copies_remaining > 0 ) {
            item map_added = get_map().add_item_or_charges( pos_bub(), it, copies_remaining );
            added = added ? added : item_location( map_cursor( pos() ), &map_added );
        }
    }
    return added;
}

ret_val<item_location> Character::i_add_or_fill( item &it, bool should_stack, const item *avoid,
        const item *original_inventory_item, const bool allow_drop,
        const bool allow_wield, bool ignore_pkt_settings )
{
    item_location loc = item_location::nowhere;
    bool success = false;
    if( it.count_by_charges() && it.charges >= 2 && !ignore_pkt_settings ) {
        const int last_charges = it.charges;
        int new_charge = last_charges;
        this->worn.add_stash( *this, it, new_charge, false );

        if( new_charge < last_charges ) {
            it.charges = new_charge;
            success = true;
        } else {
            success = false;
        }
        if( new_charge >= 1 ) {
            if( !allow_wield || !wield( it ) ) {
                if( allow_drop ) {
                    loc = item_location( map_cursor( pos() ), &get_map().add_item_or_charges( pos(), it ) );
                }
            } else {
                loc = item_location( *this, &weapon );
            }
        }
        if( success ) {
            return ret_val<item_location>::make_success( loc );
        } else {
            return ret_val<item_location>::make_failure( loc );
        }
    } else {
        loc = i_add( it, should_stack, avoid, original_inventory_item, allow_drop, allow_wield,
                     ignore_pkt_settings );
        return ret_val<item_location>::make_success( loc );
    }
}

// Negative positions indicate weapon/clothing, 0 & positive indicate inventory
const item &Character::i_at( int position ) const
{
    if( position == -1 ) {
        return weapon;
    }
    if( position < -1 ) {
        return worn.i_at( worn_position_to_index( position ) );
    }

    return inv->find_item( position );
}

item &Character::i_at( int position )
{
    return const_cast<item &>( const_cast<const Character *>( this )->i_at( position ) );
}

item Character::i_rem( const item *it )
{
    auto tmp = remove_items_with( [&it]( const item & i ) {
        return &i == it;
    }, 1 );
    if( tmp.empty() ) {
        debugmsg( "did not found item %s to remove it!", it->tname() );
        return item();
    }
    invalidate_leak_level_cache();
    return tmp.front();
}

void Character::i_rem_keep_contents( const item *const it )
{
    i_rem( it ).spill_contents( pos() );
}

bool Character::i_add_or_drop( item &it, int qty, const item *avoid,
                               const item *original_inventory_item )
{
    bool retval = true;
    bool drop = it.made_of( phase_id::LIQUID );
    bool add = it.is_gun() || !it.is_irremovable();
    inv->assign_empty_invlet( it, *this );
    map &here = get_map();
    for( int i = 0; i < qty; ++i ) {
        drop |= !can_pickWeight( it, !get_option<bool>( "DANGEROUS_PICKUPS" ) ) || !can_pickVolume( it );
        if( drop ) {
            retval &= !here.add_item_or_charges( pos(), it ).is_null();
        } else if( add ) {
            i_add( it, true, avoid,
                   original_inventory_item, /*allow_drop=*/true, /*allow_wield=*/!has_wield_conflicts( it ) );
        }
    }

    return retval;
}

bool Character::i_drop_at( item &it, int qty )
{
    bool retval = true;
    map &here = get_map();
    for( int i = 0; i < qty; ++i ) {
        retval &= !here.add_item_or_charges( pos(), it ).is_null();
    }

    return retval;
}

static void recur_internal_locations( item_location parent, std::vector<item_location> &list )
{
    for( item *it : parent->all_items_top( item_pocket::pocket_type::CONTAINER ) ) {
        item_location child( parent, it );
        recur_internal_locations( child, list );
    }
    list.push_back( parent );
}

std::vector<item_location> outfit::all_items_loc( Character &guy )
{
    std::vector<item_location> ret;
    for( item &worn_it : worn ) {
        item_location worn_loc( guy, &worn_it );
        std::vector<item_location> worn_internal_items;
        recur_internal_locations( worn_loc, worn_internal_items );
        ret.insert( ret.end(), worn_internal_items.begin(), worn_internal_items.end() );
    }
    return ret;
}

std::vector<item_location> Character::all_items_loc()
{
    std::vector<item_location> ret;
    item_location weap_loc( *this, &weapon );
    std::vector<item_location> weapon_internal_items;
    recur_internal_locations( weap_loc, weapon_internal_items );
    ret.insert( ret.end(), weapon_internal_items.begin(), weapon_internal_items.end() );
    std::vector<item_location> outfit_items = worn.all_items_loc( *this );
    ret.insert( ret.end(), outfit_items.begin(), outfit_items.end() );
    return ret;
}

std::vector<item_location> outfit::top_items_loc( Character &guy )
{
    std::vector<item_location> ret;
    for( item &worn_it : worn ) {
        item_location worn_loc( guy, &worn_it );
        ret.push_back( worn_loc );
    }
    return ret;
}

std::vector<item_location> Character::top_items_loc()
{
    return worn.top_items_loc( *this );
}

item *Character::invlet_to_item( const int linvlet ) const
{
    // Invlets may come from curses, which may also return any kind of key codes, those being
    // of type int and they can become valid, but different characters when casted to char.
    // Example: KEY_NPAGE (returned when the player presses the page-down key) is 0x152,
    // casted to char would yield 0x52, which happens to be 'R', a valid invlet.
    if( linvlet > std::numeric_limits<char>::max() || linvlet < std::numeric_limits<char>::min() ) {
        return nullptr;
    }
    const char invlet = static_cast<char>( linvlet );
    if( is_npc() ) {
        DebugLog( D_WARNING, D_GAME ) << "Why do you need to call Character::invlet_to_position on npc " <<
                                      get_name();
    }
    item *invlet_item = nullptr;
    visit_items( [&invlet, &invlet_item]( item * it, item * ) {
        if( it->invlet == invlet ) {
            invlet_item = it;
            return VisitResponse::ABORT;
        }
        return VisitResponse::NEXT;
    } );
    return invlet_item;
}

int Character::get_item_position( const item *it ) const
{
    if( weapon.has_item( *it ) ) {
        return -1;
    }

    std::optional<int> pos = worn.get_item_position( *it );
    if( pos ) {
        return worn_position_to_index( *pos );
    }

    return inv->position_by_item( it );
}

void Character::drop( item_location loc, const tripoint &where )
{
    drop( { std::make_pair( loc, loc->count() ) }, where );
    invalidate_inventory_validity_cache();
}

void Character::drop( const drop_locations &what, const tripoint &target,
                      bool stash )
{
    if( what.empty() ) {
        return;
    }
    invalidate_leak_level_cache();
    const std::optional<vpart_reference> vp = get_map().veh_at( target ).cargo();
    if( rl_dist( pos(), target ) > 1 || !( stash || get_map().can_put_items( target ) )
        || ( vp.has_value() && vp->part().is_cleaner_on() ) ) {
        add_msg_player_or_npc( m_info, _( "You can't place items here!" ),
                               _( "<npcname> can't place items here!" ) );
        return;
    }

    const tripoint placement = target - pos();
    std::vector<drop_or_stash_item_info> items;
    for( drop_location item_pair : what ) {
        if( can_drop( *item_pair.first ).success() ) {
            items.emplace_back( item_pair.first, item_pair.second );
        }
    }
    if( stash ) {
        assign_activity( stash_activity_actor( items, placement ) );
    } else {
        assign_activity( drop_activity_actor( items, placement, /* force_ground = */ false ) );
    }
}

void Character::pick_up( const drop_locations &what )
{
    if( what.empty() ) {
        return;
    }
    invalidate_leak_level_cache();
    //todo: refactor pickup_activity_actor to just use drop_locations, also rename drop_locations
    std::vector<item_location> items;
    std::vector<int> quantities;
    for( const drop_location &dl : what ) {
        items.emplace_back( dl.first );
        quantities.emplace_back( dl.second );
    }

    assign_activity( pickup_activity_actor( items, quantities, pos(), false ) );
}

invlets_bitset Character::allocated_invlets() const
{
    invlets_bitset invlets = inv->allocated_invlets();

    visit_items( [&invlets]( item * i, item * ) -> VisitResponse {
        invlets.set( i->invlet );
        return VisitResponse::NEXT;
    } );

    invlets[0] = false;

    return invlets;
}

bool Character::has_active_item( const itype_id &id ) const
{
    return has_item_with( [id]( const item & it ) {
        return it.active && it.typeId() == id;
    } );
}

ret_val<void> Character::can_drop( const item &it ) const
{
    if( it.has_flag( flag_NO_UNWIELD ) || it.has_flag( flag_INTEGRATED ) ) {
        return ret_val<void>::make_failure( _( "You cannot drop your %s." ), it.tname() );
    }
    return ret_val<void>::make_success();
}

void Character::drop_invalid_inventory()
{
    if( cache_inventory_is_valid ) {
        return;
    }
    bool dropped_liquid = false;
    for( const std::list<item> *stack : inv->const_slice() ) {
        const item &it = stack->front();
        if( it.made_of( phase_id::LIQUID ) ) {
            dropped_liquid = true;
            get_map().add_item_or_charges( pos(), it );
            // must be last
            i_rem( &it );
        }
    }
    if( dropped_liquid ) {
        add_msg_if_player( m_bad, _( "Liquid from your inventory has leaked onto the ground." ) );
    }

    item_location weap = get_wielded_item();
    if( weap ) {
        weap.overflow();
    }
    worn.overflow( *this );
    cache_inventory_is_valid = true;
}

void outfit::holster_opts( std::vector<dispose_option> &opts, item_location obj, Character &guy )
{

    for( item &e : worn ) {
        // check for attachable subpockets first (the parent item may be defined as a holster)
        if( e.get_contents().has_additional_pockets() && e.can_contain( *obj ).success() ) {
            opts.emplace_back( dispose_option{
                string_format( _( "Store in %s" ), e.tname() ), true, e.invlet,
                guy.item_store_cost( *obj, e, false, e.insert_cost( *obj ) ),
                [&guy, &e, obj] {
                    item &it = *item_location( obj );
                    guy.store( e, it, false, e.insert_cost( it ), item_pocket::pocket_type::CONTAINER, true );
                    return !guy.has_item( it );
                }
            } );
            int pkt_idx = 0;
            for( const item *it : e.get_contents().get_added_pockets() ) {
                item_pocket *con = const_cast<item_pocket *>( e.get_contents().get_added_pocket( pkt_idx++ ) );
                if( !con || !con->can_contain( *obj ).success() ) {
                    continue;
                }
                opts.emplace_back( dispose_option{
                    string_format( "  >%s", it->tname() ), true, it->invlet,
                    guy.item_store_cost( *obj, *it, false, it->insert_cost( *it ) ),
                    [&guy, it, con, obj] {
                        item &i = *item_location( obj );
                        guy.store( con, i, false, it->insert_cost( i ) );
                        return !guy.has_item( i );
                    }
                } );
            }
        } else if( e.can_holster( *obj ) ) {
            const holster_actor *ptr = dynamic_cast<const holster_actor *>
                                       ( e.type->get_use( "holster" )->get_actor_ptr() );
            opts.emplace_back( dispose_option{
                string_format( _( "Store in %s" ), e.tname() ), true, e.invlet,
                guy.item_store_cost( *obj, e, false, e.insert_cost( *obj ) ),
                [&guy, ptr, &e, obj] {
                    // *obj by itself attempts to use the const version of the operator (in gcc9),
                    // so construct a new item_location which allows using the non-const version
                    return ptr->store( guy, e, *item_location( obj ) );
                }
            } );
        }
    }
}

bool Character::dispose_item( item_location &&obj, const std::string &prompt )
{
    uilist menu;
    menu.text = prompt.empty() ? string_format( _( "Dispose of %s" ), obj->tname() ) : prompt;
    std::vector<dispose_option> opts;

    const bool bucket = obj->will_spill() && !obj->is_container_empty();

    opts.emplace_back( dispose_option{
        bucket ? _( "Spill contents and store in inventory" ) : _( "Store in inventory" ),
        can_stash( *obj ), '1',
        item_handling_cost( *obj ),
        [this, bucket, &obj] {
            if( bucket && !obj->spill_open_pockets( *this, obj.get_item() ) )
            {
                return false;
            }

            moves -= item_handling_cost( *obj );
            this->i_add( *obj, true, &*obj, &*obj );
            obj.remove_item();
            return true;
        }
    } );

    opts.emplace_back( dispose_option{
        _( "Drop item" ), true, '2', 0, [this, &obj] {
            put_into_vehicle_or_drop( *this, item_drop_reason::deliberate, { *obj } );
            obj.remove_item();
            return true;
        }
    } );

    opts.emplace_back( dispose_option{
        bucket ? _( "Spill contents and wear item" ) : _( "Wear item" ),
        can_wear( *obj ).success(), '3', item_wear_cost( *obj ),
        [this, bucket, &obj] {
            if( bucket && !obj->spill_contents( *this ) )
            {
                return false;
            }

            item it = *obj;
            obj.remove_item();
            return !!wear_item( it );
        }
    } );

    worn.holster_opts( opts, obj, *this );

    int w = utf8_width( menu.text, true ) + 4;
    for( const dispose_option &e : opts ) {
        w = std::max( w, utf8_width( e.prompt, true ) + 4 );
    }
    for( dispose_option &e : opts ) {
        e.prompt += std::string( w - utf8_width( e.prompt, true ), ' ' );
    }

    menu.text.insert( 0, 2, ' ' ); // add space for UI hotkeys
    menu.text += std::string( w + 2 - utf8_width( menu.text, true ), ' ' );
    menu.text += _( " | Moves  " );

    for( const dispose_option &e : opts ) {
        menu.addentry( -1, e.enabled, e.invlet, string_format( e.enabled ? "%s | %-7d" : "%s |",
                       e.prompt, e.moves ) );
    }

    menu.query();
    if( menu.ret >= 0 ) {
        return opts[menu.ret].action();
    }
    return false;
}

