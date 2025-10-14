item_location Character::get_wielded_item() const
{
    return const_cast<Character *>( this )->get_wielded_item();
}

item_location Character::get_wielded_item()
{
    if( weapon.is_null() ) {
        return item_location();
    }
    return item_location( *this, &weapon );
}

void Character::set_wielded_item( const item &to_wield )
{
    weapon = to_wield;
}

bool Character::has_alarm_clock() const
{
    map &here = get_map();
    if( cache_has_item_with_flag( flag_ALARMCLOCK, true ) ) {
        return true;
    }
    const optional_vpart_position &vp = here.veh_at( pos_bub() );
    return ( vp && !empty( vp->vehicle().get_avail_parts( "ALARMCLOCK" ) ) ) ||
           has_flag( json_flag_ALARMCLOCK );
}

bool Character::has_watch() const
{
    map &here = get_map();
    if( cache_has_item_with_flag( flag_WATCH, true ) ) {
        return true;
    }
    const optional_vpart_position &vp = here.veh_at( pos_bub() );
    return ( vp && !empty( vp->vehicle().get_avail_parts( "WATCH" ) ) ) ||
           has_flag( json_flag_WATCH );
}

bool Character::can_stash( const item &it, bool ignore_pkt_settings )
{
    return best_pocket( it, nullptr, ignore_pkt_settings ).second != nullptr;
}

bool Character::can_stash( const item &it, int &copies_remaining, bool ignore_pkt_settings )
{
    bool stashed_any = false;

    for( item_location loc : top_items_loc() ) {
        if( loc->can_contain( it, copies_remaining, false, false, ignore_pkt_settings ).success() ) {
            stashed_any = true;
        }
        if( copies_remaining <= 0 ) {
            return stashed_any;
        }
    }
    return stashed_any;
}

bool Character::can_stash_partial( const item &it, int &copies_remaining, bool ignore_pkt_settings )
{
    item copy = it;
    if( it.count_by_charges() ) {
        copy.charges = 1;
    }

    return can_stash( copy, copies_remaining, ignore_pkt_settings );
}

bool Character::can_stash_partial( const item &it, bool ignore_pkt_settings )
{
    item copy = it;
    if( it.count_by_charges() ) {
        copy.charges = 1;
    }

    return can_stash( copy, ignore_pkt_settings );
}

void Character::flag_encumbrance()
{
    check_encumbrance = true;
}

void Character::check_item_encumbrance_flag()
{
    if( worn.check_item_encumbrance_flag( check_encumbrance ) ) {
        calc_encumbrance();
    }
}

bool Character::natural_attack_restricted_on( const bodypart_id &bp ) const
{
    return worn.natural_attack_restricted_on( bp );
}

bool Character::natural_attack_restricted_on( const sub_bodypart_id &bp ) const
{
    return worn.natural_attack_restricted_on( bp );
}

std::vector<const item *> Character::get_pseudo_items() const
{
    if( !pseudo_items_valid ) {
        pseudo_items.clear();

        for( const bionic &bio : *my_bionics ) {
            std::vector<const item *> pseudos = bio.get_available_pseudo_items();
            for( const item *pseudo : pseudos ) {
                pseudo_items.push_back( pseudo );
            }
        }

        pseudo_items_valid = true;
    }
    return pseudo_items;
}

std::list<item> Character::remove_worn_items_with( const std::function<bool( item & )> &filter )
{
    invalidate_inventory_validity_cache();
    invalidate_leak_level_cache();
    return worn.remove_worn_items_with( filter, *this );
}

void Character::clear_worn()
{
    worn.worn.clear();
    inv_search_caches.clear();
}

std::list<item *> Character::get_dependent_worn_items( const item &it )
{
    std::list<item *> dependent;

    if( is_worn( it ) ) {
        worn.add_dependent_item( dependent, it );
    }

    return dependent;
}

item Character::remove_weapon()
{
    item tmp = weapon;
    weapon = item();
    get_event_bus().send<event_type::character_wields_item>( getID(), weapon.typeId() );
    cached_info.erase( "weapon_value" );
    invalidate_weight_carried_cache();
    return tmp;
}

void Character::remove_mission_items( int mission_id )
{
    if( mission_id == -1 ) {
        return;
    }
    remove_items_with( has_mission_item_filter { mission_id } );
}

units::mass Character::weight_carried() const
{
    if( cached_weight_carried ) {
        return *cached_weight_carried;
    }
    cached_weight_carried = weight_carried_with_tweaks( item_tweaks() );
    return *cached_weight_carried;
}

void Character::invalidate_weight_carried_cache()
{
    cached_weight_carried = std::nullopt;
}

units::mass Character::weight_carried_with_tweaks( const std::vector<std::pair<item_location, int>>
        &locations ) const
{
    std::map<const item *, int> dropping;
    for( const std::pair<item_location, int> &location_pair : locations ) {
        dropping.emplace( location_pair.first.get_item(), location_pair.second );
    }
    return weight_carried_with_tweaks( item_tweaks( { dropping } ) );
}

units::mass Character::weight_carried_with_tweaks( const item_tweaks &tweaks ) const
{
    const std::map<const item *, int> empty;
    const std::map<const item *, int> &without = tweaks.without_items ? tweaks.without_items->get() :
            empty;

    // Worn items
    units::mass ret = worn.weight_carried_with_tweaks( without );

    // Wielded item
    units::mass weaponweight = 0_gram;
    if( !without.count( &weapon ) ) {
        weaponweight += weapon.weight();
        for( const item *i : weapon.all_items_ptr( pocket_type::CONTAINER ) ) {
            if( i->count_by_charges() ) {
                weaponweight -= get_selected_stack_weight( i, without );
            } else if( without.count( i ) ) {
                weaponweight -= i->weight();
            }
        }
    } else if( weapon.count_by_charges() ) {
        weaponweight += weapon.weight() - get_selected_stack_weight( &weapon, without );
    }

    // Exclude wielded item if using lifting tool
    if( ( weaponweight + ret <= weight_capacity() ) || ( g->new_game ||
            best_nearby_lifting_assist() < weaponweight ) ) {
        ret += weaponweight;
        using_lifting_assist = false;
    } else {
        using_lifting_assist = true;
    }

    return ret;
}

units::volume Character::volume_carried_with_tweaks( const
        std::vector<std::pair<item_location, int>>
        &locations ) const
{
    std::map<const item *, int> dropping;
    for( const std::pair<item_location, int> &location_pair : locations ) {
        dropping.emplace( location_pair.first.get_item(), location_pair.second );
    }
    return volume_carried_with_tweaks( item_tweaks( { dropping } ) );
}

units::volume Character::volume_carried_with_tweaks( const item_tweaks &tweaks ) const
{
    const std::map<const item *, int> empty;
    const std::map<const item *, int> &without = tweaks.without_items ? tweaks.without_items->get() :
            empty;

    // Worn items
    units::volume ret = worn.contents_volume_with_tweaks( without );

    // Wielded item
    if( !without.count( &weapon ) ) {
        ret += weapon.get_contents_volume_with_tweaks( without );
    }

    return ret;
}

bool Character::can_pickVolume( const item &it, bool, const item *avoid,
                                const bool ignore_pkt_settings ) const
{
    if( ( avoid == nullptr || &weapon != avoid ) &&
        weapon.can_contain( it, false, false, ignore_pkt_settings ).success() ) {
        return true;
    }
    if( worn.can_pickVolume( it, ignore_pkt_settings ) ) {
        return true;
    }
    return false;
}

bool Character::can_pickVolume_partial( const item &it, bool, const item *avoid,
                                        const bool ignore_pkt_settings, const bool ignore_non_container_pocket ) const
{
    item copy = it;
    if( it.count_by_charges() ) {
        copy.charges = 1;
    }

    if( ( avoid == nullptr || &weapon != avoid ) &&
        weapon.can_contain( copy, false, false, ignore_pkt_settings, ignore_non_container_pocket
                          ).success() ) {
        return true;
    }

    return worn.can_pickVolume( copy, ignore_pkt_settings, ignore_non_container_pocket );
}

bool Character::can_pickWeight( const item &it, bool safe ) const
{
    if( !safe ) {
        return ( weight_carried() + it.weight() <= max_pickup_capacity() );
    } else {
        return ( weight_carried() + it.weight() <= weight_capacity() );
    }
}

bool Character::can_pickWeight_partial( const item &it, bool safe ) const
{
    item copy = it;
    if( it.count_by_charges() ) {
        copy.charges = 1;
    }

    return can_pickWeight( copy, safe );
}

ret_val<void> Character::can_unwield( const item &it ) const
{
    if( it.has_flag( flag_NO_UNWIELD ) ) {
        // check if "it" is currently wielded fake bionic weapon that can be deactivated
        std::optional<bionic *> bio_opt = find_bionic_by_uid( get_weapon_bionic_uid() );
        if( !is_wielding( it ) || it.ethereal || !bio_opt ||
            !can_deactivate_bionic( **bio_opt ).success() ) {
            return ret_val<void>::make_failure( _( "You can't unwield your %s." ), it.tname() );
        }
    }

    return ret_val<void>::make_success();
}

void Character::invalidate_inventory_validity_cache()
{
    cache_inventory_is_valid = false;
}
bool Character::is_wielding( const item &target ) const
{
    return &weapon == &target;
}

bool Character::has_worn_module_with_flag( const flag_id &f )
{
    std::vector<item *> flag_items = cache_get_items_with( f );
    bool has_flag = std::any_of( flag_items.begin(), flag_items.end(),
    [this]( item * i ) {
        return is_worn_module( *i );
    } );
    return has_flag;
}

void Character::calc_encumbrance()
{
    calc_encumbrance( item() );
}

void Character::calc_encumbrance( const item &new_item )
{
    std::map<bodypart_id, encumbrance_data> enc;
    worn.item_encumb( enc, new_item, *this );
    mut_cbm_encumb( enc );
    calc_bmi_encumb( enc );

    for( const std::pair<const bodypart_id, encumbrance_data> &elem : enc ) {
        set_part_encumbrance_data( elem.first, elem.second );
    }

}

void layer_details::reset()
{
    *this = layer_details();
}

// The stacking penalty applies by doubling the encumbrance of
// each item except the highest encumbrance one.
// So we add them together and then subtract out the highest.
int layer_details::layer( const int encumbrance, bool conflicts )
{
    /*
     * We should only get to this point with an encumbrance value of 0
     * if the item is 'semitangible'. A normal item has a minimum of
     * 2 encumbrance for layer penalty purposes.
     * ( even if normally its encumbrance is 0 )
     */
    if( encumbrance == 0 ) {
        return total; // skip over the other logic because this item doesn't count
    }

    // if this layer has conflicting pieces then add that information
    if( conflicts ) {
        is_conflicting = true;
    }

    pieces.push_back( encumbrance );

    int current = total;
    if( encumbrance > max ) {
        total += max;   // *now* the old max is counted, just ignore the new max
        max = encumbrance;
    } else {
        total += encumbrance;
    }
    return total - current;
}

body_part_set Character::exclusive_flag_coverage( const flag_id &flag ) const
{
    body_part_set ret;
    ret.fill( get_all_body_parts() );
    return worn.exclusive_flag_coverage( ret, flag );
}

item Character::reduce_charges( item *it, int quantity )
{
    if( !has_item( *it ) ) {
        debugmsg( "invalid item (name %s) for reduce_charges", it->tname() );
        return item();
    }
    if( it->charges <= quantity ) {
        return i_rem( it );
    }
    it->mod_charges( -quantity );
    item result( *it );
    result.charges = quantity;
    return result;
}

bool Character::has_mission_item( int mission_id ) const
{
    return mission_id != -1 && has_item_with( has_mission_item_filter{ mission_id } );
}

void Character::toolmod_add( item_location tool, item_location mod )
{
    if( !tool && !mod ) {
        debugmsg( "Tried toolmod installation but mod/tool not available" );
        return;
    }
    // first check at least the minimum requirements are met
    if( !has_trait( trait_DEBUG_HS ) && !can_use( *mod, *tool ) ) {
        return;
    }

    if( mod->is_irremovable() ) {
        if( !query_yn( _( "Permanently install your %1$s in your %2$s?" ),
                       colorize( mod->tname(), mod->color_in_inventory() ),
                       colorize( tool->tname(), tool->color_in_inventory() ) ) ) {
            add_msg_if_player( _( "Never mind." ) );
            return; // player canceled installation
        }
    }

    assign_activity( ACT_TOOLMOD_ADD, 1, -1 );
    activity.targets.emplace_back( std::move( tool ) );
    activity.targets.emplace_back( std::move( mod ) );
}

int Character::item_handling_cost( const item &it, bool penalties, int base_cost,
                                   int charges_in_it, bool bulk_cost ) const
{
    int mv = base_cost;
    if( penalties ) {
        // 50 moves per liter, up to 200 at 4 liters
        mv += std::min( 200, it.volume( false, false, charges_in_it ) / 20_ml );
    }

    if( weapon.typeId() == itype_e_handcuffs ) {
        mv *= 4;
    } else if( penalties && has_flag( json_flag_GRAB ) ) {
        // Grabbed penalty scales for grabbed arms/hands
        int pen = 2;
        for( const effect &eff : get_effects_with_flag( json_flag_GRAB ) ) {
            if( eff.get_bp()->primary_limb_type() == bp_type::arm ||
                eff.get_bp()->primary_limb_type() == bp_type::hand ) {
                pen++;
            }
        }
        mv *= pen;
    }

    // The following is considered as overhead and is not included in the cost when handling multiple items at once.
    if( !bulk_cost ) {
        // For single handed items use the least encumbered hand
        if( it.is_two_handed( *this ) ) {
            for( const bodypart_id &part : get_all_body_parts_of_type( bp_type::hand ) ) {
                mv += encumb( part );
            }
        } else {
            int min_encumb = INT_MAX;
            for( const bodypart_id &part : get_all_body_parts_of_type( bp_type::hand ) ) {
                min_encumb = std::min( min_encumb, encumb( part ) );
            }
            mv += min_encumb;
        }
    }

    return std::max( mv, 0 );
}

int Character::item_store_cost( const item &it, const item & /* container */, bool penalties,
                                int base_cost ) const
{
    /** @EFFECT_PISTOL decreases time taken to store a pistol */
    /** @EFFECT_SMG decreases time taken to store an SMG */
    /** @EFFECT_RIFLE decreases time taken to store a rifle */
    /** @EFFECT_SHOTGUN decreases time taken to store a shotgun */
    /** @EFFECT_LAUNCHER decreases time taken to store a launcher */
    /** @EFFECT_STABBING decreases time taken to store a stabbing weapon */
    /** @EFFECT_CUTTING decreases time taken to store a cutting weapon */
    /** @EFFECT_BASHING decreases time taken to store a bashing weapon */
    float lvl = get_skill_level( it.is_gun() ? it.gun_skill() : it.melee_skill() );
    return item_handling_cost( it, penalties, base_cost ) / ( ( lvl + 10.0f ) / 10.0f );
}

int Character::item_retrieve_cost( const item &it, const item &container, bool penalties,
                                   int base_cost ) const
{
    // Drawing from an holster use the same formula as storing an item for now
    /**
         * @EFFECT_PISTOL decreases time taken to draw pistols from holsters
         * @EFFECT_SMG decreases time taken to draw smgs from holsters
         * @EFFECT_RIFLE decreases time taken to draw rifles from holsters
         * @EFFECT_SHOTGUN decreases time taken to draw shotguns from holsters
         * @EFFECT_LAUNCHER decreases time taken to draw launchers from holsters
         * @EFFECT_STABBING decreases time taken to draw stabbing weapons from sheathes
         * @EFFECT_CUTTING decreases time taken to draw cutting weapons from scabbards
         * @EFFECT_BASHING decreases time taken to draw bashing weapons from holsters
         */
    return item_store_cost( it, container, penalties, base_cost );
}

ret_val<void> Character::can_wield( const item &it ) const
{
    if( it.has_flag( flag_INTEGRATED ) ) {
        return ret_val<void>::make_failure( _( "You can't wield a part of your body." ) );
    }
    if( has_effect( effect_incorporeal ) ) {
        return ret_val<void>::make_failure( _( "You can't wield anything while incorporeal." ) );
    }
    if( !has_min_manipulators() ) {
        return ret_val<void>::make_failure(
                   _( "You need at least one arm available to even consider wielding something." ) );
    }
    if( it.made_of( phase_id::LIQUID ) ) {
        return ret_val<void>::make_failure( _( "You can't wield spilt liquids." ) );
    }
    if( it.made_of( phase_id::GAS ) ) {
        return ret_val<void>::make_failure( _( "You can't wield loose gases." ) );
    }
    if( it.is_frozen_liquid() && !it.has_flag( flag_SHREDDED ) ) {
        return ret_val<void>::make_failure( _( "You can't wield unbroken frozen liquids." ) );
    }
    if( it.has_flag( flag_NO_UNWIELD ) ) {
        if( get_wielded_item() && get_wielded_item().get_item() == &it ) {
            return ret_val<void>::make_failure( _( "You can't unwield this." ) );
        }
        return ret_val<void>::make_failure(
                   _( "You can't wield this.  Wielding it would make it impossible to unwield it." ) );
    }
    if( it.has_flag( flag_BIONIC_WEAPON ) ) {
        return ret_val<void>::make_failure(
                   _( "You can't wield this.  It looks like it has to be attached to a bionic." ) );
    }

    if( is_armed() && !can_unwield( weapon ).success() ) {
        return ret_val<void>::make_failure( _( "The %1$s prevents you from wielding the %2$s." ),
                                            weapname(), it.tname() );
    }
    monster *mount = mounted_creature.get();
    const itype_id mech_weapon = is_mounted() ? mount->type->mech_weapon : itype_id::NULL_ID();
    bool mounted_mech = is_mounted() && mount->has_flag( mon_flag_RIDEABLE_MECH ) &&
                        mech_weapon;
    bool armor_restricts_hands = worn_with_flag( flag_RESTRICT_HANDS );
    bool item_twohand = it.has_flag( flag_ALWAYS_TWOHAND );
    bool missing_arms = !has_two_arms_lifting();
    bool two_handed = it.is_two_handed( *this );

    if( two_handed &&
        ( missing_arms || armor_restricts_hands ) &&
        !( mounted_mech && it.typeId() == mech_weapon ) //ignore this check for mech weapons
      ) {
        if( armor_restricts_hands ) {
            return ret_val<void>::make_failure(
                       _( "Something you are wearing hinders the use of both hands." ) );
        } else if( item_twohand ) {
            return ret_val<void>::make_failure( _( "You can't wield the %s with only one arm." ),
                                                it.tname() );
        } else {
            return ret_val<void>::make_failure( _( "You are too weak to wield the %s with only one arm." ),
                                                it.tname() );
        }
    }
    if( mounted_mech && it.typeId() != mech_weapon ) {
        return ret_val<void>::make_failure( _( "You cannot wield anything while piloting a mech." ) );
    }
    if( controlling_vehicle ) {
        if( two_handed ) {
            return ret_val<void>::make_failure( _( "You need both hands to wield the %s but are driving." ),
                                                it.tname() );
        }
        if( armor_restricts_hands ) {
            return ret_val<void>::make_failure(
                       _( "Something you are wearing hinders the use of both hands." ) );
        }
        if( missing_arms || item_twohand ) {
            return ret_val<void>::make_failure( _( "You can't wield your %s while driving." ),
                                                it.tname() );
        }
    }

    return ret_val<void>::make_success();
}

bool Character::has_wield_conflicts( const item &it ) const
{
    return is_wielding( it ) || ( is_armed() && !it.can_combine( weapon ) );
}

bool Character::unwield()
{
    if( weapon.is_null() ) {
        return true;
    }

    if( !can_unwield( weapon ).success() ) {
        return false;
    }

    // currently the only way to unwield NO_UNWIELD weapon is if it's a bionic that can be deactivated
    if( weapon.has_flag( flag_NO_UNWIELD ) ) {
        std::optional<bionic *> bio_opt = find_bionic_by_uid( get_weapon_bionic_uid() );
        return bio_opt ? deactivate_bionic( **bio_opt ) : false;
    }

    const std::string query = string_format( _( "Stop wielding %s?" ), weapon.tname() );

    if( !dispose_item( item_location( *this, &weapon ), query ) ) {
        return false;
    }

    inv->unsort();

    return true;
}

std::string Character::weapname() const
{
    std::string name = weapname_simple();
    const std::string mode = weapname_mode();
    const std::string ammo = weapname_ammo();

    if( !mode.empty() ) {
        name = string_format( "%s %s", mode, name );
    }
    if( !ammo.empty() ) {
        name = string_format( "%s %s", name, ammo );
    }

    return name;
}

std::string Character::weapname_simple() const
{
    //To make wield state consistent, gun_nam; when calling tname, is disabling 'with_collapsed' flag
    if( weapon.is_gun() ) {
        gun_mode current_mode = weapon.gun_current_mode();
        const bool no_mode = !current_mode.target;
        tname::segment_bitset segs( tname::default_tname );
        segs.set( tname::segments::TAGS, false );
        std::string gun_name = no_mode ? weapon.display_name() : current_mode->tname( 1, segs );
        return gun_name;

    } else if( !is_armed() ) {
        return _( "fists" );
    } else {
        return weapon.tname();
    }
}

std::string Character::weapname_mode() const
{
    if( weapon.is_gun() ) {
        gun_mode current_mode = weapon.gun_current_mode();
        const bool no_mode = !current_mode.target;
        std::string gunmode;
        if( !no_mode && current_mode->gun_all_modes().size() > 1 ) {
            gunmode = current_mode.tname();
        }
        return gunmode;
    } else {
        return "";
    }
}

std::string Character::weapname_ammo() const
{
    if( weapon.is_gun() ) {
        gun_mode current_mode = weapon.gun_current_mode();
        const bool no_mode = !current_mode.target;
        // only required for empty mags and empty guns
        std::string mag_ammo;

        if( !no_mode && ( current_mode->uses_magazine() || current_mode->magazine_integral() ) ) {
            if( current_mode->uses_magazine() && !current_mode->magazine_current() ) {
                mag_ammo = _( "(empty)" );
            } else {
                int cur_ammo = current_mode->ammo_remaining( );
                int max_ammo;
                if( cur_ammo == 0 ) {
                    max_ammo = current_mode->ammo_capacity( item( current_mode->ammo_default() ).ammo_type() );
                } else {
                    max_ammo = current_mode->ammo_capacity( current_mode->loaded_ammo().ammo_type() );
                }

                const double ratio = static_cast<double>( cur_ammo ) / static_cast<double>( max_ammo );
                nc_color charges_color;
                if( cur_ammo == 0 ) {
                    charges_color = c_light_red;
                } else if( cur_ammo == max_ammo ) {
                    charges_color = c_white;
                } else if( ratio < 1.0 / 3.0 ) {
                    charges_color = c_red;
                } else if( ratio < 2.0 / 3.0 ) {
                    charges_color = c_yellow;
                } else {
                    charges_color = c_light_green;
                }
                mag_ammo = string_format( "(%s)", colorize( string_format( "%i/%i", cur_ammo, max_ammo ),
                                          charges_color ) );
            }
        }

        return mag_ammo;

    } else {
        return "";
    }
}

std::vector<item *> Character::inv_dump()
{
    std::vector<item *> ret;
    if( is_armed() && can_drop( weapon ).success() ) {
        ret.push_back( &weapon );
    }
    worn.inv_dump( ret );
    inv->dump( ret );
    return ret;
}

std::vector<const item *> Character::inv_dump() const
{
    std::vector<const item *> ret;
    if( is_armed() && can_drop( weapon ).success() ) {
        ret.push_back( &weapon );
    }
    worn.inv_dump( ret );
    inv->dump( ret );
    return ret;
}

bool Character::covered_with_flag( const flag_id &f, const body_part_set &parts ) const
{
    if( parts.none() ) {
        return true;
    }

    return worn.covered_with_flag( f, parts );
}

bool Character::is_waterproof( const body_part_set &parts ) const
{
    return covered_with_flag( flag_WATERPROOF, parts );
}

units::volume Character::free_space() const
{
    units::volume volume_capacity = 0_ml;
    volume_capacity += weapon.get_total_capacity();
    for( const item_pocket *pocket : weapon.get_all_contained_pockets() ) {
        if( pocket->contains_phase( phase_id::SOLID ) ) {
            for( const item *it : pocket->all_items_top() ) {
                volume_capacity -= it->volume();
            }
        } else if( !pocket->empty() ) {
            volume_capacity -= pocket->volume_capacity();
        }
    }
    volume_capacity += weapon.check_for_free_space();
    volume_capacity += worn.free_space();
    return volume_capacity;
}

units::mass Character::free_weight_capacity() const
{
    units::mass weight_capacity = 0_gram;
    weight_capacity += weapon.get_remaining_weight_capacity();
    weight_capacity += worn.free_weight_capacity();
    return weight_capacity;
}

units::volume Character::holster_volume() const
{
    units::volume holster_volume = 0_ml;
    if( weapon.is_holster() ) {
        holster_volume += weapon.get_total_capacity();
    }
    holster_volume += worn.holster_volume();
    return holster_volume;
}

int Character::empty_holsters() const
{
    int e_holsters = 0;
    if( weapon.is_holster() ) {
        e_holsters += 1;
    }
    e_holsters += worn.empty_holsters();
    return e_holsters;
}

int Character::used_holsters() const
{
    int e_holsters = 0;
    if( weapon.is_holster() ) {
        e_holsters += weapon.get_used_holsters();
    }
    e_holsters += worn.used_holsters();
    return e_holsters;
}

int Character::total_holsters() const
{
    int e_holsters = 0;
    if( weapon.is_holster() ) {
        e_holsters += weapon.get_total_holsters();
    }
    e_holsters += worn.total_holsters();
    return e_holsters;
}

units::volume Character::free_holster_volume() const
{
    units::volume holster_volume = 0_ml;
    if( weapon.is_holster() ) {
        holster_volume += weapon.get_total_holster_volume() - weapon.get_used_holster_volume();
    }
    holster_volume += worn.free_holster_volume();
    return holster_volume;
}

units::volume Character::small_pocket_volume( const units::volume &threshold ) const
{
    units::volume small_spaces = 0_ml;
    if( weapon.get_total_capacity() <= threshold ) {
        small_spaces += weapon.get_total_capacity();
    }
    small_spaces += worn.small_pocket_volume( threshold );
    return small_spaces;
}

units::volume Character::volume_capacity() const
{
    units::volume volume_capacity = 0_ml;
    volume_capacity += weapon.get_total_capacity();
    volume_capacity += worn.volume_capacity();
    return volume_capacity;
}

units::volume Character::volume_capacity_with_tweaks( const
        std::vector<std::pair<item_location, int>>
        &locations ) const
{
    std::map<const item *, int> dropping;
    for( const std::pair<item_location, int> &location_pair : locations ) {
        dropping.emplace( location_pair.first.get_item(), location_pair.second );
    }
    return volume_capacity_with_tweaks( item_tweaks( { dropping } ) );
}

units::volume Character::volume_capacity_with_tweaks( const item_tweaks &tweaks ) const
{
    const std::map<const item *, int> empty;
    const std::map<const item *, int> &without = tweaks.without_items ? tweaks.without_items->get() :
            empty;

    units::volume volume_capacity = 0_ml;

    if( !without.count( &weapon ) ) {
        volume_capacity += weapon.get_total_capacity();
    }

    volume_capacity += worn.volume_capacity_with_tweaks( without );

    return volume_capacity;
}

units::volume Character::volume_carried() const
{
    return volume_capacity() - free_space();
}

void Character::toggle_hauling()
{
    map &here = get_map();

    if( hauling ) {
        stop_hauling();
    } else {
        std::vector<item_location> items = here.get_haulable_items( pos_bub() );
        if( items.empty() ) {
            add_msg( m_info, _( "There are no items to haul here." ) );
            return;
        }
        start_hauling( items );
        start_autohaul();
    }
}

void Character::start_hauling( const std::vector<item_location> &items_to_haul = {} )
{
    map &here = get_map();

    if( here.veh_at( pos_bub() ) ) {
        add_msg( m_info, _( "You cannot haul inside vehicles." ) );
        return;
    } else if( here.has_flag( ter_furn_flag::TFLAG_DEEP_WATER, pos_bub() ) ) {
        add_msg( m_info, _( "You cannot haul while in deep water." ) );
        return;
    } else if( !here.can_put_items( pos_bub() ) || here.impassable_field_at( pos_bub() ) ) {
        add_msg( m_info, _( "You cannot haul items here." ) );
        return;
    }

    add_msg( _( "You start hauling items along the ground." ) );
    if( is_armed() ) {
        add_msg( m_warning, _( "Your hands are not free, which makes hauling slower." ) );
    }

    hauling = true;
    haul_list = items_to_haul;
}

void Character::stop_hauling()
{
    if( hauling ) {
        add_msg( _( "You stop hauling items." ) );
        hauling = false;
        autohaul = false;
        haul_list.clear();
    }
    if( has_activity( ACT_MOVE_ITEMS ) ) {
        cancel_activity();
    }
}

bool Character::is_hauling() const
{
    return hauling;
}

void Character::start_autohaul()
{
    autohaul = true;
    if( !is_hauling() ) {
        start_hauling();
    }
}

void Character::stop_autohaul()
{
    autohaul = false;
    if( haul_list.empty() ) {
        stop_hauling();
    }
}

bool Character::is_autohauling() const
{
    return autohaul;
}

bool Character::trim_haul_list( const std::vector<item_location> &valid_items )
{
    size_t qty_before = haul_list.size();
    haul_list.erase( std::remove_if( haul_list.begin(),
    haul_list.end(), [&valid_items]( const item_location & it ) {
        return std::count( valid_items.begin(), valid_items.end(), it ) == 0;
    } ), haul_list.end() );

    return qty_before != haul_list.size();
}

void Character::migrate_items_to_storage( bool disintegrate )
{
    inv->visit_items( [&]( const item * it, item * ) {
        if( disintegrate ) {
            if( try_add( *it, /*avoid=*/nullptr, it ) == item_location::nowhere ) {
                std::string profession_id = prof->ident().str();
                debugmsg( "ERROR: Could not put %s (%s) into inventory.  Check if the "
                          "profession (%s) has enough space.",
                          it->tname(), it->typeId().str(), profession_id );
                return VisitResponse::ABORT;
            }
        } else {
            item_location added = i_add( *it, true, /*avoid=*/nullptr,
                                         it, /*allow_drop=*/false, /*allow_wield=*/!has_wield_conflicts( *it ) );
            if( added == item_location::nowhere ) {
                put_into_vehicle_or_drop( *this, item_drop_reason::tumbling, { *it } );
            }
        }
        return VisitResponse::SKIP;
    } );
    inv->clear();
}

std::string Character::is_snuggling() const
{
    map &here = get_map();
    map_stack items_here = here.i_at( pos_bub() );
    item_stack::const_iterator begin = items_here.begin();
    item_stack::const_iterator end = items_here.end();

    if( in_vehicle ) {
        if( const std::optional<vpart_reference> ovp = here.veh_at( pos_bub() ).cargo() ) {
            const vehicle_stack vs = ovp->items();
            begin = vs.begin();
            end = vs.end();
        }
    }
    const item *floor_armor = nullptr;
    int ticker = 0;

    // If there are no items on the floor, return nothing
    if( begin == end ) {
        return "nothing";
    }

    for( auto candidate = begin; candidate != end; ++candidate ) {
        if( !candidate->is_armor() ) {
            continue;
        } else if( candidate->volume() > 250_ml && candidate->get_warmth() > 0 &&
                   ( candidate->covers( body_part_torso ) || candidate->covers( body_part_leg_l ) ||
                     candidate->covers( body_part_leg_r ) ) ) {
            floor_armor = &*candidate;
            ticker++;
        }
    }

    if( ticker == 0 ) {
        return "nothing";
    } else if( ticker == 1 ) {
        return floor_armor->type_name();
    } else if( ticker > 1 ) {
        return "many";
    }

    return "nothing";
}

// If the player is not wielding anything big, check if hands can be put in pockets
bool Character::can_use_pockets() const
{
    // TODO Check that the pocket actually has enough space for the wielded item?
    return weapon.volume() < 500_ml;
}

// If the player's head is not encumbered, check if hood can be put up
bool Character::can_use_hood() const
{
    return encumb( body_part_head ) < 10;
}

// If the player's mouth is not encumbered, check if collar can be put up
bool Character::can_use_collar() const
{
    return encumb( body_part_mouth ) < 10;
}

void Character::cache_visit_items_with( const itype_id &type,
                                        const std::function<void( item & )> &do_func )
{
    cache_visit_items_with( "HAS TYPE " + type.str(), type, {}, nullptr, do_func );
}

void Character::cache_visit_items_with( const flag_id &type_flag,
                                        const std::function<void( item & )> &do_func )
{
    cache_visit_items_with( "HAS FLAG " + type_flag.str(), {}, type_flag, nullptr, do_func );
}

void Character::cache_visit_items_with( const std::string &key, bool( item::*filter_func )() const,
                                        const std::function<void( item & )> &do_func )
{
    cache_visit_items_with( key, {}, {}, filter_func, do_func );
}

void Character::cache_visit_items_with( const std::string &key, const itype_id &type,
                                        const flag_id &type_flag, bool( item::*filter_func )() const,
                                        const std::function<void( item & )> &do_func )
{
    // If the cache already exists, use it. Remove all invalid item references.
    auto found_cache = inv_search_caches.find( key );
    if( found_cache != inv_search_caches.end() ) {
        inv_search_caches[key].items.erase( std::remove_if( inv_search_caches[key].items.begin(),
        inv_search_caches[key].items.end(), [&do_func]( const safe_reference<item> &it ) {
            if( it ) {
                do_func( *it );
                return false;
            }
            return true;
        } ), inv_search_caches[key].items.end() );
        return;
    } else {
        // Otherwise, add a new cache and populate with all appropriate items in the inventory. Empty lists are still created.
        inv_search_caches[key].type = type;
        inv_search_caches[key].type_flag = type_flag;
        inv_search_caches[key].filter_func = filter_func;
        visit_items( [&]( item * it, item * ) {
            if( ( !type.is_valid() || it->typeId() == type ) &&
                ( !type_flag.is_valid() || it->type->has_flag( type_flag ) ) &&
                ( filter_func == nullptr || ( it->*filter_func )() ) ) {

                inv_search_caches[key].items.push_back( it->get_safe_reference() );
                do_func( *it );
            }
            return VisitResponse::NEXT;
        } );
    }
}

void Character::cache_visit_items_with( const itype_id &type,
                                        const std::function<void( const item & )> &do_func ) const
{
    cache_visit_items_with( "HAS TYPE " + type.str(), type, {}, nullptr, do_func );
}

void Character::cache_visit_items_with( const flag_id &type_flag,
                                        const std::function<void( const item & )> &do_func ) const
{
    cache_visit_items_with( "HAS FLAG " + type_flag.str(), {}, type_flag, nullptr, do_func );
}

void Character::cache_visit_items_with( const std::string &key, bool( item::*filter_func )() const,
                                        const std::function<void( const item & )> &do_func ) const
{
    cache_visit_items_with( key, {}, {}, filter_func, do_func );
}

void Character::cache_visit_items_with( const std::string &key, const itype_id &type,
                                        const flag_id &type_flag, bool( item::*filter_func )() const,
                                        const std::function<void( const item & )> &do_func ) const
{
    // If the cache already exists, use it. Remove all invalid item references.
    auto found_cache = inv_search_caches.find( key );
    if( found_cache != inv_search_caches.end() ) {
        inv_search_caches[key].items.erase( std::remove_if( inv_search_caches[key].items.begin(),
        inv_search_caches[key].items.end(), [&do_func]( const safe_reference<item> &it ) {
            if( it ) {
                do_func( *it );
                return false;
            }
            return true;
        } ), inv_search_caches[key].items.end() );
        return;
    } else {
        // Otherwise, add a new cache and populate with all appropriate items in the inventory. Empty lists are still created.
        inv_search_caches[key].type = type;
        inv_search_caches[key].type_flag = type_flag;
        inv_search_caches[key].filter_func = filter_func;
        visit_items( [&]( item * it, item * ) {
            if( ( !type.is_valid() || it->typeId() == type ) &&
                ( !type_flag.is_valid() || it->type->has_flag( type_flag ) ) &&
                ( filter_func == nullptr || ( it->*filter_func )() ) ) {

                inv_search_caches[key].items.push_back( it->get_safe_reference() );
                do_func( *it );
            }
            return VisitResponse::NEXT;
        } );
    }
}

bool Character::cache_has_item_with( const itype_id &type,
                                     const std::function<bool( const item & )> &check_func ) const
{
    return cache_has_item_with( "HAS TYPE " + type.str(), type, {}, nullptr, check_func );
}

bool Character::cache_has_item_with( const flag_id &type_flag,
                                     const std::function<bool( const item & )> &check_func ) const
{
    return cache_has_item_with( "HAS FLAG " + type_flag.str(), {}, type_flag, nullptr, check_func );
}

bool Character::cache_has_item_with( const std::string &key, bool( item::*filter_func )() const,
                                     const std::function<bool( const item & )> &check_func ) const
{
    return cache_has_item_with( key, {}, {}, filter_func, check_func );
}

bool Character::cache_has_item_with( const std::string &key, const itype_id &type,
                                     const flag_id &type_flag, bool( item::*filter_func )() const,
                                     const std::function<bool( const item & )> &check_func ) const
{
    bool aborted = false;

    // If the cache already exists, use it. Stop iterating if the check_func ever returns true. Remove any invalid item references encountered.
    auto found_cache = inv_search_caches.find( key );
    if( found_cache != inv_search_caches.end() ) {
        for( auto iter = found_cache->second.items.begin();
             iter != found_cache->second.items.end(); ) {
            if( *iter ) {
                if( check_func( **iter ) ) {
                    aborted = true;
                    break;
                }
                ++iter;
            } else {
                iter = inv_search_caches[found_cache->first].items.erase( iter );
            }
        }
    } else {
        // Otherwise, add a new cache and populate with all appropriate items in the inventory. Empty lists are still created.
        inv_search_caches[key].type = type;
        inv_search_caches[key].type_flag = type_flag;
        inv_search_caches[key].filter_func = filter_func;
        visit_items( [&]( item * it, item * ) {
            if( ( !type.is_valid() || it->typeId() == type ) &&
                ( !type_flag.is_valid() || it->type->has_flag( type_flag ) ) &&
                ( filter_func == nullptr || ( it->*filter_func )() ) ) {

                inv_search_caches[key].items.push_back( it->get_safe_reference() );
                // If check_func returns true, stop running it but keep populating the cache.
                if( !aborted && check_func( *it ) ) {
                    aborted = true;
                }
            }
            return VisitResponse::NEXT;
        } );
    }
    return aborted;
}

bool Character::has_item_with_flag( const flag_id &flag, bool need_charges ) const
{
    return has_item_with( [&flag, &need_charges, this]( const item & it ) {
        return it.has_flag( flag ) && ( !need_charges || !it.is_tool() ||
                                        it.type->tool->max_charges == 0 || it.ammo_remaining( this ) > 0 );
    } );
}

bool Character::cache_has_item_with_flag( const flag_id &type_flag, bool need_charges ) const
{
    return cache_has_item_with( "HAS FLAG " + type_flag.str(), {}, type_flag, nullptr,
    [this, &need_charges]( const item & it ) {
        return !need_charges || !it.is_tool() || it.type->tool->max_charges == 0 ||
               it.ammo_remaining( this ) > 0;
    } );
}

std::vector<item *> Character::cache_get_items_with( const itype_id &type,
        const std::function<bool( item & )> &do_and_check_func )
{
    return cache_get_items_with( "HAS TYPE " + type.str(), type, {}, nullptr, do_and_check_func );
}

std::vector<item *> Character::cache_get_items_with( const flag_id &type_flag,
        const std::function<bool( item & )> &do_and_check_func )
{
    return cache_get_items_with( "HAS FLAG " + type_flag.str(), {}, type_flag, nullptr,
                                 do_and_check_func );
}

std::vector<item *> Character::cache_get_items_with( const std::string &key,
        bool( item::*filter_func )() const,
        const std::function<bool( item & )> &do_and_check_func )
{
    return cache_get_items_with( key, {}, {}, filter_func, do_and_check_func );
}

std::vector<item *> Character::cache_get_items_with( const std::string &key, const itype_id &type,
        const flag_id &type_flag, bool( item::*filter_func )() const,
        const std::function<bool( item & )> &do_and_check_func )
{
    std::vector<item *> ret;
    cache_visit_items_with( key, type, type_flag, filter_func, [&ret, &do_and_check_func]( item & it ) {
        if( do_and_check_func( it ) ) {
            ret.push_back( &it );
        }
    } );
    return ret;
}

std::vector<const item *> Character::cache_get_items_with( const itype_id &type,
        const std::function<bool( const item & )> &check_func ) const
{
    return cache_get_items_with( "HAS TYPE " + type.str(), type, {}, nullptr, check_func );
}

std::vector<const item *> Character::cache_get_items_with( const flag_id &type_flag,
        const std::function<bool( const item & )> &check_func ) const
{
    return cache_get_items_with( "HAS FLAG " + type_flag.str(), {}, type_flag, nullptr, check_func );
}

std::vector<const item *> Character::cache_get_items_with( const std::string &key,
        bool( item::*filter_func )() const,
        const std::function<bool( const item & )> &check_func ) const
{
    return cache_get_items_with( key, {}, {}, filter_func, check_func );
}

std::vector<const item *> Character::cache_get_items_with( const std::string &key,
        const itype_id &type, const flag_id &type_flag, bool( item::*filter_func )() const,
        const std::function<bool( const item & )> &check_func ) const
{
    std::vector<const item *> ret;
    cache_visit_items_with( key, type, type_flag, filter_func, [&ret, &check_func]( const item & it ) {
        if( check_func( it ) ) {
            ret.push_back( &it );
        }
    } );
    return ret;
}

void Character::add_to_inv_search_caches( item &it ) const
{
    for( auto &cache : inv_search_caches ) {
        if( ( cache.second.type.is_valid() && it.typeId() != cache.second.type ) ||
            ( cache.second.type_flag.is_valid() && !it.type->has_flag( cache.second.type_flag ) ) ||
            ( cache.second.filter_func && !( it.*cache.second.filter_func )() ) ) {
            continue;
        }

        // If item is already in the cache, remove it so it can be re-added in its current state.
        for( auto iter = cache.second.items.begin(); iter != cache.second.items.end(); ) {
            if( *iter && iter->get() == &it ) {
                iter = inv_search_caches[cache.first].items.erase( iter );
            } else {
                ++iter;
            }
        }

        cache.second.items.push_back( it.get_safe_reference() );
    }
}

void Character::clear_inventory_search_cache()
{
    inv_search_caches.clear();
}

bool Character::has_charges( const itype_id &it, int quantity,
                             const std::function<bool( const item & )> &filter ) const
{
    if( it == itype_fire || it == itype_apparatus ) {
        return has_fire( quantity );
    }
    return charges_of( it, quantity, filter ) == quantity;
}

item Character::find_firestarter_with_charges( const int quantity ) const
{
    item ret;
    cache_has_item_with( flag_FIRESTARTER, [&]( const item & it ) {
        if( !it.typeId()->can_have_charges() ) {
            const use_function *usef = it.type->get_use( "firestarter" );
            if( usef != nullptr && usef->get_actor_ptr() != nullptr ) {
                const firestarter_actor *actor = dynamic_cast<const firestarter_actor *>( usef->get_actor_ptr() );
                if( actor->can_use( *this->as_character(), it, tripoint_bub_ms::zero ).success() ) {
                    ret = it;
                    return true;
                }
            }
        } else if( it.ammo_sufficient( this, quantity ) ) {
            ret = it;
            return true;
        }
        return false;
    } );
    return ret;
}

bool Character::has_fire( const int quantity ) const
{
    // TODO: Replace this with a "tool produces fire" flag.

    if( get_map().has_nearby_fire( pos_bub() ) ) {
        return true;
    }
    if( cache_has_item_with( flag_FIRE ) ) {
        return true;
    }
    if( !find_firestarter_with_charges( quantity ).is_null() ) {
        return true;
    }
    if( is_npc() ) {
        // HACK: A hack to make NPCs use their Molotovs
        return true;
    }

    return false;
}

void Character::on_worn_item_washed( const item &it )
{
    if( is_worn( it ) ) {
        morale->on_worn_item_washed( it );
    }
}

void Character::on_item_wear( const item &it )
{
    invalidate_inventory_validity_cache();
    invalidate_leak_level_cache();
    morale->on_item_wear( it );
}

void Character::on_item_takeoff( const item &it )
{
    invalidate_inventory_validity_cache();
    invalidate_weight_carried_cache();
    invalidate_leak_level_cache();
    morale->on_item_takeoff( it );
}

void Character::on_item_acquire( const item &it )
{
    bool check_for_zoom = is_avatar();
    bool update_overmap_seen = false;

    it.visit_items( [this, &check_for_zoom, &update_overmap_seen]( item * cont_it, item * ) {
        add_to_inv_search_caches( *cont_it );
        if( check_for_zoom && !update_overmap_seen && cont_it->has_flag( flag_ZOOM ) ) {
            update_overmap_seen = true;
        }
        return VisitResponse::NEXT;
    } );

    invalidate_weight_carried_cache();

    if( update_overmap_seen ) {
        g->update_overmap_seen();
    }
}

item &Character::best_item_with_quality( const quality_id &qid )
{
    int max_lvl_found = INT_MIN;
    std::vector<item *> items = items_with( [qid, &max_lvl_found]( const item & it ) {
        int qlvl = it.get_quality_nonrecursive( qid );
        if( qlvl > max_lvl_found ) {
            max_lvl_found = qlvl;
            return true;
        }
        return false;
    } );
    if( max_lvl_found > INT_MIN ) {
        return *items.back();
    }
    return null_item_reference();
}

bool Character::add_or_drop_with_msg( item &it, const bool /*unloading*/, const item *avoid,
                                      const item *original_inventory_item )
{
    if( it.made_of( phase_id::LIQUID ) ) {
        liquid_handler::consume_liquid( it, 1, avoid );
        return it.charges <= 0;
    }
    if( !this->can_pickVolume( it, false, avoid ) ) {
        put_into_vehicle_or_drop( *this, item_drop_reason::too_large, { it } );
    } else if( !this->can_pickWeight( it, !get_option<bool>( "DANGEROUS_PICKUPS" ) ) ) {
        put_into_vehicle_or_drop( *this, item_drop_reason::too_heavy, { it } );
    } else {
        bool wielded_has_it = false;
        // Cannot use weapon.has_item(it) because it skips any pockets that
        // are not containers such as magazines and magazine wells.
        for( const item *scan_contents : weapon.all_items_top() ) {
            if( scan_contents == &it ) {
                wielded_has_it = true;
                break;
            }
        }
        const bool allow_wield = !wielded_has_it && weapon.magazine_current() != &it;
        const int prev_charges = it.charges;
        item_location ni = i_add( it, true, avoid,
                                  original_inventory_item, /*allow_drop=*/false, /*allow_wield=*/allow_wield );
        if( ni == item_location::nowhere ) {
            // failed to add
            put_into_vehicle_or_drop( *this, item_drop_reason::tumbling, { it } );
        } else if( &*ni == &it ) {
            // merged into the original stack, restore original charges
            it.charges = prev_charges;
            put_into_vehicle_or_drop( *this, item_drop_reason::tumbling, { it } );
        } else {
            // successfully added
            add_msg( _( "You put the %s in your inventory." ), ni->tname() );
            add_msg( m_info, "%c - %s", ni->invlet == 0 ? ' ' : ni->invlet, ni->tname() );
        }
    }
    return true;
}

bool Character::unload( item_location &loc, bool bypass_activity,
                        const item_location &new_container )
{
    item &it = *loc;
    drop_locations locs;
    // Unload a container consuming moves per item successfully removed
    if( it.is_container() ) {
        if( it.empty() ) {
            add_msg( m_info, _( "The %s is already empty!" ), it.tname() );
            return false;
        }
        if( !it.can_unload() ) {
            add_msg( m_info, _( "The item can't be unloaded in its current state!" ) );
            return false;
        }

        int moves = 0;
        item *prev_contained = nullptr;

        for( pocket_type ptype : {
                 pocket_type::CONTAINER,
                 pocket_type::MAGAZINE_WELL,
                 pocket_type::MAGAZINE
             } ) {

            for( item *contained : it.all_items_top( ptype, true ) ) {
                if( new_container == item_location::nowhere ) {
                    if( prev_contained && prev_contained->stacks_with( *contained ) ) {
                        moves += std::max( this->item_handling_cost( *contained, true, 0, -1, true ), 1 );
                    } else {
                        moves += this->item_handling_cost( *contained );
                    }
                    prev_contained = contained;
                } else {
                    // redirect to insert actor
                    const int count = contained->count_by_charges() ? contained->charges : 1;
                    locs.emplace_back( item_location( loc, contained ), count );
                }
            }

        }
        if( new_container == item_location::nowhere ) {
            assign_activity( unload_activity_actor( moves, loc ) );
        } else if( !locs.empty() ) {
            assign_activity( insert_item_activity_actor( new_container, locs, true ) );
        }

        return true;
    }

    // If item can be unloaded more than once (currently only guns) prompt user to choose
    std::vector<std::string> msgs( 1, it.tname() );
    std::vector<item *> opts( 1, &it );

    for( item *e : it.gunmods() ) {
        if( ( e->is_gun() && !e->has_flag( flag_NO_UNLOAD ) &&
              ( e->magazine_current() || e->ammo_remaining( ) > 0 || e->casings_count() > 0 ) ) ||
            ( e->has_flag( flag_BRASS_CATCHER ) && !e->is_container_empty() ) ) {
            msgs.emplace_back( e->tname() );
            opts.emplace_back( e );
        }
    }

    item *target = nullptr;
    item_location targloc;
    if( opts.size() > 1 ) {
        const int ret = uilist( _( "Unload what?" ), msgs );
        if( ret >= 0 ) {
            target = opts[ret];
            targloc = item_location( loc, opts[ret] );
        }
    } else {
        target = &it;
        targloc = loc;
    }

    if( target == nullptr ) {
        return false;
    }

    // Next check for any reasons why the item cannot be unloaded
    if( !target->has_flag( flag_BRASS_CATCHER ) ) {
        if( !target->is_magazine() && !target->uses_magazine() ) {
            add_msg( m_info, _( "You can't unload a %s!" ), target->tname() );
            return false;
        }

        if( target->has_flag( flag_NO_UNLOAD ) ) {
            if( target->has_flag( flag_RECHARGE ) || target->has_flag( flag_USE_UPS ) ) {
                add_msg( m_info, _( "You can't unload a rechargeable %s!" ), target->tname() );
            } else {
                add_msg( m_info, _( "You can't unload a %s!" ), target->tname() );
            }
            return false;
        }

        if( !target->magazine_current() && target->ammo_remaining( ) <= 0 &&
            target->casings_count() <= 0 ) {
            if( target->is_tool() ) {
                add_msg( m_info, _( "Your %s isn't charged." ), target->tname() );
            } else {
                add_msg( m_info, _( "Your %s isn't loaded." ), target->tname() );
            }
            return false;
        }
    }

    target->casings_handle( [&]( item & e ) {
        return this->i_add_or_drop( e );
    } );

    invalidate_weight_carried_cache();

    if( target->is_magazine() ) {
        if( bypass_activity ) {
            unload_activity_actor::unload( *this, targloc );
        } else {
            int mv = 0;
            for( const item *content : target->all_items_top() ) {
                // We use the same cost for both reloading and unloading
                mv += this->item_reload_cost( it, *content, content->charges );
            }
            if( it.is_ammo_belt() ) {
                // Disassembling ammo belts is easier than assembling them
                mv /= 2;
            }
            assign_activity( unload_activity_actor( mv, targloc ) );
        }
        return true;

    } else if( target->magazine_current() ) {
        if( !this->add_or_drop_with_msg( *target->magazine_current(), true, nullptr,
                                         target->magazine_current() ) ) {
            return false;
        }
        // Eject magazine consuming half as much time as required to insert it
        this->mod_moves( -this->item_reload_cost( *target, *target->magazine_current(), -1 ) / 2 );

        target->remove_items_with( [&target]( const item & e ) {
            return target->magazine_current() == &e;
        } );

    } else if( target->ammo_remaining( ) ) {
        int qty = target->ammo_remaining( );

        // Construct a new ammo item and try to drop it
        item ammo( target->ammo_current(), calendar::turn, qty );
        if( target->is_filthy() ) {
            ammo.set_flag( flag_FILTHY );
        }

        if( ammo.made_of_from_type( phase_id::LIQUID ) ) {
            if( !this->add_or_drop_with_msg( ammo ) ) {
                qty -= ammo.charges; // only handled part (or none) of the liquid
            }
            if( qty <= 0 ) {
                return false; // no liquid was moved
            }

        } else if( !this->add_or_drop_with_msg( ammo, qty > 1 ) ) {
            return false;
        }

        // If successful remove appropriate qty of ammo consuming half as much time as required to load it
        this->mod_moves( -this->item_reload_cost( *target, ammo, qty ) / 2 );

        target->ammo_set( target->ammo_current(), target->ammo_remaining( ) - qty );
    } else if( target->has_flag( flag_BRASS_CATCHER ) ) {
        target->spill_contents( get_player_character() );
    }

    // Turn off any active tools
    if( target->is_tool() && target->active && target->ammo_remaining( ) == 0 ) {
        target->deactivate( this );
    }

    add_msg( _( "You unload your %s." ), target->tname() );

    if( it.has_flag( flag_MAG_DESTROY ) && it.ammo_remaining( ) == 0 ) {
        loc.remove_item();
    }

    get_player_character().recoil = MAX_RECOIL;

    return true;
}

void Character::invalidate_pseudo_items()
{
    pseudo_items_valid = false;
}

void Character::on_worn_item_transform( const item &old_it, const item &new_it )
{
    morale->on_worn_item_transform( old_it, new_it );
}

void Character::leak_items()
{
    map &here = get_map();

    std::vector<item_location> removed_items;
    if( weapon.is_container() ) {
        if( weapon.leak( here, this, pos_bub() ) ) {
            weapon.spill_contents( pos_bub() );
        }
    } else if( weapon.made_of( phase_id::LIQUID ) ) {
        if( weapon.leak( here, this, pos_bub() ) ) {
            here.add_item_or_charges( pos_bub(), weapon );
            removed_items.emplace_back( *this, &weapon );
            add_msg_if_player( m_warning, _( "%s spilled from your hand." ), weapon.tname() );
        }
    }

    for( item_location it : top_items_loc() ) {
        if( !it || ( !it->is_container() && !it->made_of( phase_id::LIQUID ) ) ) {
            continue;
        }
        if( it->leak( here, this, pos_bub() ) ) {
            it->spill_contents( pos_bub() );
            removed_items.push_back( it );
        }
    }
    for( item_location removed : removed_items ) {
        removed.remove_item();
    }
}

void Character::process_items( map *here )
{
    if( weapon.process( *here, this, pos_bub( *here ) ) ) {
        weapon.spill_contents( here,  pos_bub( *here ) );
        remove_weapon();
    }

    std::vector<item_location> removed_items;
    for( item_location it : top_items_loc() ) {
        if( !it ) {
            continue;
        }
        if( it->process( *here, this, pos_bub( *here ) ) ) {
            it->spill_contents( here, pos_bub( *here ) );
            removed_items.push_back( it );
        }
    }
    for( item_location removed : removed_items ) {
        removed.remove_item();
    }

    // Active item processing done, now we're recharging.
    if( worn.check_item_encumbrance_flag( get_check_encumbrance() ) ) {
        calc_encumbrance();
        set_check_encumbrance( false );
    }

    // Load all items that use the UPS and have their own battery to their minimal functional charge,
    // The tool is not really useful if its charges are below charges_to_use
    std::vector<item *> inv_use_ups = cache_get_items_with( flag_USE_UPS, [&here]( item & it ) {
        return ( it.ammo_capacity( ammo_battery ) > it.ammo_remaining_linked( *here, nullptr ) ||
                 ( it.type->battery && it.type->battery->max_capacity > it.energy_remaining( nullptr ) ) );
    } );
    if( !inv_use_ups.empty() ) {
        const units::energy available_charges = available_ups();
        units::energy ups_used = 0_kJ;
        for( item * const &it : inv_use_ups ) {
            // For powered armor, an armor-powering bionic should always be preferred over UPS usage.
            if( it->is_power_armor() && can_interface_armor() && has_power() ) {
                // Bionic power costs are handled elsewhere
                continue;
            } else if( it->active && !it->ammo_sufficient( this ) ) {
                it->deactivate();
                add_msg_if_player( _( "Your %s shut down due to lack of power." ), it->tname() );
            } else if( available_charges - ups_used >= 1_kJ &&
                       it->ammo_remaining_linked( *here, nullptr ) < it->ammo_capacity( ammo_battery ) ) {
                // Charge the battery in the UPS modded tool
                ups_used += 1_kJ;
                it->ammo_set( itype_battery, it->ammo_remaining_linked( *here, nullptr ) + 1 );
            }
        }
        if( ups_used > 0_kJ ) {
            consume_ups( ups_used );
        }
    }
}

void Character::search_surroundings()
{
    if( controlling_vehicle ) {
        return;
    }
    if( has_effect( effect_subaquatic_sonar ) && is_underwater() && calendar::once_every( 4_turns ) ) {
        echo_pulse();
    }
    map &here = get_map();
    // Search for traps in a larger area than before because this is the only
    // way we can "find" traps that aren't marked as visible.
    // Detection formula takes care of likelihood of seeing within this range.
    for( const tripoint_bub_ms &tp : here.points_in_radius( pos_bub(), 5 ) ) {
        const trap &tr = here.tr_at( tp );
        if( tr.is_null() || tp == pos_bub() ) {
            continue;
        }
        // Note that echolocation and SONAR also do this separately in echo_pulse()
        if( has_active_bionic( bio_ground_sonar ) && !knows_trap( tp ) && tr.detected_by_ground_sonar() ) {
            const std::string direction = direction_name( direction_from( pos_bub(), tp ) );
            add_msg_if_player( m_warning, _( "Your ground sonar detected a %1$s to the %2$s!" ),
                               tr.name(), direction );
            add_known_trap( tp, tr );
        }
        if( !sees( here, tp ) ) {
            continue;
        }
        if( tr.can_see( tp, *this ) ) {
            // Already seen, or can never be seen
            continue;
        }
        // Chance to detect traps we haven't yet seen.
        if( tr.detect_trap( tp, *this ) ) {
            if( !tr.is_trivial_to_spot() ) {
                // Only bug player about traps that aren't trivial to spot.
                const std::string direction = direction_name(
                                                  direction_from( pos_bub(), tp ) );
                practice_proficiency( proficiency_prof_spotting, 1_minutes );
                // Seeing a trap set properly gives you a little bonus to trapsetting profs.
                practice_proficiency( proficiency_prof_traps, 10_seconds );
                practice_proficiency( proficiency_prof_trapsetting, 10_seconds );
                add_msg_if_player( _( "You've spotted a %1$s to the %2$s!" ),
                                   tr.name(), direction );
            }
            add_known_trap( tp, tr );
        }
    }
}

bool Character::wield( item &it, std::optional<int> obtain_cost, bool combat )
{
    invalidate_inventory_validity_cache();
    invalidate_leak_level_cache();

    item_location wielded = get_wielded_item();

    if( has_wield_conflicts( it ) ) {
        const bool is_unwielding = is_wielding( it );
        const auto ret = can_unwield( it );

        if( !ret.success() ) {
            add_msg_if_player( m_info, "%s", ret.c_str() );
            return false;
        }

        if( !is_unwielding && wielded->has_item( it ) ) {
            add_msg_if_player( m_info,
                               _( "You need to put the bag away before trying to wield something from it." ) );
            return false;
        }

        if( !unwield() ) {
            return false;
        }

        if( is_unwielding ) {
            if( !martial_arts_data->selected_is_none() ) {
                martial_arts_data->martialart_use_message( *this );
            }
            return true;
        }
    }

    if( !can_wield( it ).success() ) {
        return false;
    }

    bool combine_stacks = wielded && it.can_combine( *wielded );
    cached_info.erase( "weapon_value" );

    // Wielding from inventory is relatively slow and does not improve with increasing weapon skill.
    // Worn items (including guns with shoulder straps) are faster but still slower
    // than a skilled player with a holster.
    // There is an additional penalty when wielding items from the inventory whilst currently grabbed.

    bool worn = is_worn( it );

    // Ideally the cost should be calculated from wield(item_location), but as backup try it here.
    const int mv = obtain_cost.value_or( item_handling_cost( it, true,
                                         INVENTORY_HANDLING_PENALTY / ( worn ? 2 : 1 ) ) );

    if( worn ) {
        it.on_takeoff( *this );
    }

    add_msg_debug( debugmode::DF_AVATAR, "wielding took %d moves", mv );
    mod_moves( -mv );

    item to_wield;
    if( has_item( it ) ) {
        to_wield = i_rem( &it );
    } else {
        // is_null means fists
        to_wield = it.is_null() ? item() : it;
    }

    if( combine_stacks ) {
        wielded->combine( to_wield );
    } else {
        set_wielded_item( to_wield );
    }

    // set_wielded_item invalidates the weapon item_location, so get it again
    wielded = get_wielded_item();
    recoil = MAX_RECOIL;

    // if fists are wielded get_wielded_item returns item_location::nowhere, which is a nullptr
    if( wielded ) {
        last_item = wielded->typeId();
        wielded->on_wield( *this, combat );
        inv->update_invlet( *wielded );
        inv->update_cache_with_item( *wielded );
        cata::event e = cata::event::make<event_type::character_wields_item>( getID(), last_item );
        get_event_bus().send_with_talker( this, &wielded, e );
    } else {
        last_item = to_wield.typeId();
        get_event_bus().send<event_type::character_wields_item>( getID(), item().typeId() );
    }
    return true;
}

bool Character::wield( item_location loc, bool remove_old )
{
    if( !loc ) {
        add_msg_if_player( _( "No item." ) );
        return false;
    }

    if( !wield( *loc, loc.obtain_cost( *this ) ) ) {
        return false;
    }

    if( remove_old && loc ) {
        // Remove DROPPED_FAVORITES autonote if exists.
        if( overmap_buffer.note( pos_abs_omt() ) == loc.get_item()->display_name() ) {
            overmap_buffer.delete_note( pos_abs_omt() );
        }

        loc.remove_item();
    }
    return true;
}

bool Character::wield_contents( item &container, item *internal_item, bool penalties,
                                int base_cost )
{
    // if index not specified and container has multiple items then ask the player to choose one
    if( internal_item == nullptr ) {
        debugmsg( "No valid target for wield contents." );
        return false;
    }

    if( !container.has_item( *internal_item ) ) {
        debugmsg( "Tried to wield non-existent item from container (Character::wield_contents)" );
        return false;
    }

    const ret_val<void> ret = can_wield( *internal_item );
    if( !ret.success() ) {
        add_msg_if_player( m_info, "%s", ret.c_str() );
        return false;
    }

    int mv = 0;

    if( has_wield_conflicts( *internal_item ) ) {
        if( !unwield() ) {
            return false;
        }
        inv->unsort();
    }

    // for holsters, we should not include the cost of wielding the holster itself
    // The cost of wielding the holster was already added earlier in avatar_action::use_item.
    // As we couldn't make sure back then what action was going to be used, we remove the cost now.
    item_location il = item_location( *this, &container );
    mv -= il.obtain_cost( *this );
    mv += item_retrieve_cost( *internal_item, container, penalties, base_cost );

    if( internal_item->stacks_with( weapon, true ) ) {
        weapon.combine( *internal_item );
    } else {
        weapon = std::move( *internal_item );
    }
    container.remove_item( *internal_item );
    container.on_contents_changed();

    inv->update_invlet( weapon );
    inv->update_cache_with_item( weapon );
    last_item = weapon.typeId();

    mod_moves( -mv );

    weapon.on_wield( *this );

    item_location loc( *this, &weapon );
    cata::event e = cata::event::make<event_type::character_wields_item>( getID(), weapon.typeId() );
    get_event_bus().send_with_talker( this, &loc, e );

    return true;
}

void Character::store( item &container, item &put, bool penalties, int base_cost,
                       pocket_type pk_type, bool check_best_pkt )
{
    mod_moves( -item_store_cost( put, container, penalties, base_cost ) );
    if( check_best_pkt && pk_type == pocket_type::CONTAINER &&
        container.get_all_contained_pockets().size() > 1 ) {
        // Bypass pocket settings (assuming the item is manually stored)
        int charges = put.count_by_charges() ? put.charges : 1;
        container.fill_with( i_rem( &put ), charges, false, false, true );
    } else {
        container.put_in( i_rem( &put ), pk_type );
    }
    calc_encumbrance();
}

void Character::store( item_pocket *pocket, item &put, bool penalties, int base_cost )
{
    if( !pocket ) {
        return;
    }

    item_location char_item( *this, &null_item_reference() );
    item_pocket *pkt_best = pocket->best_pocket_in_contents( char_item, put, nullptr, false,
                            false ).second;
    if( !!pkt_best && pocket->better_pocket( *pkt_best, put, true ) ) {
        pocket = pkt_best;
    }
    mod_moves( -std::max( item_store_cost( put, null_item_reference(), penalties, base_cost ),
                          pocket->obtain_cost( put ) ) );
    ret_val<item *> result = pocket->insert_item( i_rem( &put ) );
    result.value()->on_pickup( *this );
    calc_encumbrance();
}
