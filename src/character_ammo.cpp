#include "ammo.h"

#include <utility>

#include "character.h"
#include "character_modifier.h"
#include "flag.h"
#include "game.h"
#include "itype.h"
#include "output.h"

static const character_modifier_id character_modifier_reloading_move_mod( "reloading_move_mod" );
static const itype_id itype_battery( "battery" );
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

item::reload_option Character::select_ammo( const item_location &base,
        std::vector<item::reload_option> opts, const std::string &name_override ) const
{
    if( opts.empty() ) {
        add_msg_if_player( m_info, _( "Never mind." ) );
        return item::reload_option();
    }

    std::string name = name_override.empty() ? base->tname() : name_override;
    uilist menu;
    menu.text = string_format( base->is_watertight_container() ? _( "Refill %s" ) :
                               base->has_flag( flag_RELOAD_AND_SHOOT ) ? _( "Select ammo for %s" ) : _( "Reload %s" ),
                               name );

    // Construct item names
    std::vector<std::string> names;
    std::transform( opts.begin(), opts.end(),
    std::back_inserter( names ), [&]( const item::reload_option & e ) {
        if( e.ammo->is_magazine() && e.ammo->ammo_data() ) {
            if( e.ammo->ammo_current() == itype_battery ) {
                // This battery ammo is not a real object that can be recovered but pseudo-object that represents charge
                //~ battery storage (charges)
                return string_format( pgettext( "magazine", "%1$s (%2$d)" ), e.ammo->type_name(),
                                      e.ammo->ammo_remaining() );
            } else {
                //~ magazine with ammo (count)
                return string_format( pgettext( "magazine", "%1$s with %2$s (%3$d)" ), e.ammo->type_name(),
                                      e.ammo->ammo_data()->nname( e.ammo->ammo_remaining() ), e.ammo->ammo_remaining() );
            }
        } else if( e.ammo->is_watertight_container() ||
                   ( e.ammo->is_ammo_container() && is_worn( *e.ammo ) ) ) {
            // worn ammo containers should be named by their ammo contents with their location also updated below
            return e.ammo->first_ammo().display_name();

        } else {
            return ( ammo_location && ammo_location == e.ammo ? "* " : "" ) + e.ammo->display_name();
        }
    } );

    // Get location descriptions
    std::vector<std::string> where;
    std::transform( opts.begin(), opts.end(),
    std::back_inserter( where ), [this]( const item::reload_option & e ) {
        bool is_ammo_container = e.ammo->is_ammo_container();
        Character &player_character = get_player_character();
        if( is_ammo_container || e.ammo->is_container() ) {
            if( is_ammo_container && is_worn( *e.ammo ) ) {
                return e.ammo->type_name();
            }
            return string_format( _( "%s, %s" ), e.ammo->type_name(), e.ammo.describe( &player_character ) );
        }
        return e.ammo.describe( &player_character );
    } );
    // Get destination names
    std::vector<std::string> destination;
    std::transform( opts.begin(), opts.end(),
    std::back_inserter( destination ), [&]( const item::reload_option & e ) {
        name = name_override.empty() ? e.target->tname( 1, false, 0, false ) :
               name_override;
        if( ( e.target->is_gunmod() || e.target->is_magazine() ) && e.target.has_parent() ) {
            return string_format( _( "%s in %s" ), name, e.target.parent_item()->tname( 1, false, 0, false ) );
        } else {
            return name;
        }
    } );
    // Pads elements to match longest member and return length
    auto pad = []( std::vector<std::string> &vec, int n, int t ) -> int {
        for( const auto &e : vec )
        {
            n = std::max( n, utf8_width( e, true ) + t );
        }
        for( auto &e : vec )
        {
            e += std::string( n - utf8_width( e, true ), ' ' );
        }
        return n;
    };

    // Pad the first column including 4 trailing spaces
    int w = pad( names, utf8_width( menu.text, true ), 6 );
    menu.text.insert( 0, 2, ' ' ); // add space for UI hotkeys
    menu.text += std::string( w + 2 - utf8_width( menu.text, true ), ' ' );

    // Pad the location similarly (excludes leading "| " and trailing " ")
    w = pad( where, utf8_width( _( "| Location " ) ) - 3, 6 );
    menu.text += _( "| Location " );
    menu.text += std::string( w + 3 - utf8_width( _( "| Location " ) ), ' ' );

    // Pad the names of target
    w = pad( destination, utf8_width( _( "| Destination " ) ) - 3, 6 );
    menu.text += _( "| Destination " );
    menu.text += std::string( w + 3 - utf8_width( _( "| Destination " ) ), ' ' );

    menu.text += _( "| Amount  " );
    menu.text += _( "| Moves   " );

    // We only show ammo statistics for guns and magazines
    if( ( base->is_gun() || base->is_magazine() ) && !base->is_tool() ) {
        menu.text += _( "| Damage  | Pierce  " );
    }

    auto draw_row = [&]( int idx ) {
        const item::reload_option &sel = opts[ idx ];
        std::string row = string_format( "%s| %s | %s |", names[ idx ], where[ idx ], destination[ idx ] );
        row += string_format( ( sel.ammo->is_ammo() ||
                                sel.ammo->is_ammo_container() ) ? " %-7d |" : "         |", sel.qty() );
        row += string_format( " %-7d ", sel.moves() );

        if( ( base->is_gun() || base->is_magazine() ) && !base->is_tool() ) {
            const itype *ammo = sel.ammo->is_ammo_container() ? sel.ammo->first_ammo().ammo_data() :
                                sel.ammo->ammo_data();
            if( ammo ) {
                const damage_instance &dam = ammo->ammo->damage;
                row += string_format( "| %-7d | %-7d", static_cast<int>( dam.total_damage() ),
                                      static_cast<int>( dam.empty() ? 0.0f : ( *dam.begin() ).res_pen ) );
            } else {
                row += "|         |         ";
            }
        }
        return row;
    };

    const ammotype base_ammotype( base->ammo_default().str() );
    itype_id last = uistate.lastreload[ base_ammotype ];
    // We keep the last key so that pressing the key twice (for example, r-r for reload)
    // will always pick the first option on the list.
    int last_key = inp_mngr.get_previously_pressed_key();
    bool last_key_bound = false;
    // This is the entry that has out default
    int default_to = 0;

    // If last_key is RETURN, don't use that to override hotkey
    if( last_key == '\n' ) {
        last_key_bound = true;
        default_to = -1;
    }

    for( int i = 0; i < static_cast<int>( opts.size() ); ++i ) {
        const item &ammo = opts[ i ].ammo->is_ammo_container() ? opts[ i ].ammo->first_ammo() :
                           *opts[ i ].ammo;

        char hotkey = -1;
        if( has_item( ammo ) ) {
            // if ammo in player possession and either it or any container has a valid invlet use this
            if( ammo.invlet ) {
                hotkey = ammo.invlet;
            } else {
                for( const item *obj : parents( ammo ) ) {
                    if( obj->invlet ) {
                        hotkey = obj->invlet;
                        break;
                    }
                }
            }
        }
        if( last == ammo.typeId() ) {
            if( !last_key_bound && hotkey == -1 ) {
                // If this is the first occurrence of the most recently used type of ammo and the hotkey
                // was not already set above then set it to the keypress that opened this prompt
                hotkey = last_key;
                last_key_bound = true;
            }
            if( !last_key_bound ) {
                // Pressing the last key defaults to the first entry of compatible type
                default_to = i;
                last_key_bound = true;
            }
        }
        if( hotkey == last_key ) {
            last_key_bound = true;
            // Prevent the default from being used: key is bound to something already
            default_to = -1;
        }

        menu.addentry( i, true, hotkey, draw_row( i ) );
    }

    struct reload_callback : public uilist_callback {
        public:
            std::vector<item::reload_option> &opts;
            const std::function<std::string( int )> draw_row;
            int last_key;
            const int default_to;
            const bool can_partial_reload;

            reload_callback( std::vector<item::reload_option> &_opts,
                             std::function<std::string( int )> _draw_row,
                             int _last_key, int _default_to, bool _can_partial_reload ) :
                opts( _opts ), draw_row( std::move( _draw_row ) ),
                last_key( _last_key ), default_to( _default_to ),
                can_partial_reload( _can_partial_reload )
            {}

            bool key( const input_context &, const input_event &event, int idx, uilist *menu ) override {
                int cur_key = event.get_first_input();
                if( default_to != -1 && cur_key == last_key ) {
                    // Select the first entry on the list
                    menu->ret = default_to;
                    return true;
                }
                if( idx < 0 || idx >= static_cast<int>( opts.size() ) ) {
                    return false;
                }
                auto &sel = opts[ idx ];
                switch( cur_key ) {
                    case KEY_LEFT:
                        if( can_partial_reload ) {
                            sel.qty( sel.qty() - 1 );
                            menu->entries[ idx ].txt = draw_row( idx );
                        }
                        return true;

                    case KEY_RIGHT:
                        if( can_partial_reload ) {
                            sel.qty( sel.qty() + 1 );
                            menu->entries[ idx ].txt = draw_row( idx );
                        }
                        return true;
                }
                return false;
            }
    } cb( opts, draw_row, last_key, default_to, !base->has_flag( flag_RELOAD_ONE ) );
    menu.callback = &cb;

    menu.query();
    if( menu.ret < 0 || static_cast<size_t>( menu.ret ) >= opts.size() ) {
        add_msg_if_player( m_info, _( "Never mind." ) );
        return item::reload_option();
    }

    const item_location &sel = opts[ menu.ret ].ammo;
    uistate.lastreload[ base_ammotype ] = sel->is_ammo_container() ?
                                          // get first item in all magazine pockets
                                          sel->first_ammo().typeId() :
                                          sel->typeId();
    return opts[ menu.ret ];
}

item::reload_option Character::select_ammo( const item_location &base, bool prompt,
        bool empty ) const
{
    if( !base ) {
        return item::reload_option();
    }

    std::vector<item::reload_option> ammo_list;
    bool ammo_match_found = list_ammo( base, ammo_list, empty );

    if( ammo_list.empty() ) {
        if( !is_npc() ) {
            if( !base->magazine_integral() && !base->magazine_current() ) {
                add_msg_if_player( m_info, _( "You need a compatible magazine to reload the %s!" ),
                                   base->tname() );

            } else if( ammo_match_found ) {
                add_msg_if_player( m_info, _( "You can't reload anything with the ammo you have on hand." ) );
            } else {
                std::string name;
                if( base->ammo_data() ) {
                    name = base->ammo_data()->nname( 1 );
                } else if( base->is_watertight_container() ) {
                    name = base->is_container_empty() ? "liquid" : base->legacy_front().tname();
                } else {
                    const std::set<ammotype> types_of_ammo = base->ammo_types();
                    name = enumerate_as_string( types_of_ammo.begin(),
                    types_of_ammo.end(), []( const ammotype & at ) {
                        return at->name();
                    }, enumeration_conjunction::none );
                }
                if( base->is_magazine_full() ) {
                    add_msg_if_player( m_info, _( "The %s is already full!" ),
                                       base->tname() );
                } else {
                    add_msg_if_player( m_info, _( "You don't have any %s to reload your %s!" ),
                                       name, base->tname() );
                }
            }
        }
        return item::reload_option();
    }

    // sort in order of move cost (ascending), then remaining ammo (descending) with empty magazines always last
    std::stable_sort( ammo_list.begin(), ammo_list.end(), []( const item::reload_option & lhs,
    const item::reload_option & rhs ) {
        return lhs.ammo->ammo_remaining() > rhs.ammo->ammo_remaining();
    } );
    std::stable_sort( ammo_list.begin(), ammo_list.end(), []( const item::reload_option & lhs,
    const item::reload_option & rhs ) {
        return lhs.moves() < rhs.moves();
    } );
    std::stable_sort( ammo_list.begin(), ammo_list.end(), []( const item::reload_option & lhs,
    const item::reload_option & rhs ) {
        return ( lhs.ammo->ammo_remaining() != 0 ) > ( rhs.ammo->ammo_remaining() != 0 );
    } );

    if( is_npc() ) {
        if( ammo_list[0].ammo.get_item()->ammo_remaining() > 0 ) {
            return ammo_list[0];
        } else {
            return item::reload_option();
        }
    }

    if( !prompt && ammo_list.size() == 1 ) {
        // unconditionally suppress the prompt if there's only one option
        return ammo_list[ 0 ];
    }

    return select_ammo( base, std::move( ammo_list ) );
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

