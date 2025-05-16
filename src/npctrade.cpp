#include "npctrade.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <list>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "avatar.h"
#include "character.h"
#include "character_attire.h"
#include "debug.h"
#include "enums.h"
#include "faction.h"
#include "item.h"
#include "item_category.h" // IWYU pragma: keep
#include "item_contents.h"
#include "item_location.h"
#include "item_pocket.h"
#include "npc.h"
#include "npc_opinion.h"
#include "npctrade_utils.h"
#include "pocket_type.h"
#include "ret_val.h"
#include "skill.h"
#include "trade_ui.h"
#include "type_id.h"
#include "units.h"

static const flag_id json_flag_NO_UNWIELD( "NO_UNWIELD" );
static const skill_id skill_speech( "speech" );

std::list<item> npc_trading::transfer_items( trade_selector::select_t &stuff, Character &giver,
        Character &receiver, std::list<item_location *> &from_map, bool use_escrow )
{
    // escrow is used only when the npc is the destination. Item transfer to the npc is deferred.
    std::list<item> escrow = std::list<item>();

    for( trade_selector::entry_t &ip : stuff ) {
        if( ip.first.get_item() == nullptr ) {
            DebugLog( D_ERROR, D_NPC ) << "Null item being traded in npc_trading::transfer_items";
            continue;
        }
        item gift = *ip.first.get_item();

        npc const *npc = nullptr;
        std::function<bool( item_location const &, int )> f_wants;
        if( giver.is_npc() ) {
            npc = giver.as_npc();
            f_wants = [npc]( item_location const & it, int price ) {
                return npc->wants_to_sell( it, price ).success();
            };
        } else if( receiver.is_npc() ) {
            npc = receiver.as_npc();
            f_wants = [npc]( item_location const & it, int price ) {
                return npc->wants_to_buy( *it, price ).success();
            };
        }
        // spill contained, unwanted items
        if( f_wants && gift.is_container() ) {
            for( item *it : gift.get_contents().all_items_top() ) {
                int const price =
                    trading_price( giver, receiver, { item_location{ giver, it }, 1 } );
                if( !f_wants( item_location{ ip.first, it }, price ) ) {
                    giver.i_add_or_drop( *it, 1, ip.first.get_item() );
                    gift.remove_item( *it );
                }
            }
        }

        gift.set_owner( receiver );

        // Items are moving to escrow.
        if( use_escrow && ip.first->count_by_charges() ) {
            gift.charges = ip.second;
            escrow.emplace_back( gift );
        } else if( use_escrow ) {
            std::fill_n( std::back_inserter( escrow ), ip.second, gift );
            // No escrow in use. Items moving from giver to receiver.
        } else if( ip.first->count_by_charges() ) {
            gift.charges = ip.second;
            item newit = item( gift );
            ret_val<item_location> ret = receiver.i_add_or_fill( newit, true, nullptr, &gift,
                                         /*allow_drop=*/true, /*allow_wield=*/true, false );
        } else {
            for( int i = 0; i < ip.second; i++ ) {
                receiver.i_add( gift );
            }
        }

        if( ip.first.held_by( giver ) ) {
            if( ip.first->count_by_charges() ) {
                giver.use_charges( gift.typeId(), ip.second );
            } else if( ip.second > 0 ) {
                giver.remove_items_with( [&ip]( const item & i ) {
                    return &i == ip.first.get_item();
                }, ip.second );
            }
        } else {
            if( ip.first->count_by_charges() ) {
                ip.first.get_item()->set_var( "trade_charges", ip.second );
            } else {
                ip.first.get_item()->set_var( "trade_amount", 1 );
            }
            from_map.push_back( &ip.first );
        }
    }
    return escrow;
}

std::vector<item_pricing> npc_trading::init_selling( npc &np )
{
    std::vector<item_pricing> result;
    const std::vector<item *> inv_all = np.items_with( []( const item & it ) {
        return !it.made_of( phase_id::LIQUID );
    } );
    for( item *i : inv_all ) {
        item &it = *i;

        int val = np.value( it );
        // FIXME: this item_location is a hack
        if( np.wants_to_sell( item_location{ np, i }, val ).success() ) {
            result.emplace_back( np, it, val, static_cast<int>( it.count() ) );
        }
    }

    item_location weapon = np.get_wielded_item();
    if(
        np.will_exchange_items_freely() && weapon && !weapon->has_flag( json_flag_NO_UNWIELD )
    ) {
        result.emplace_back( np, *weapon, np.value( *weapon ), false );
    }

    return result;
}

double npc_trading::net_price_adjustment( const Character &buyer, const Character &seller )
{
    // Adjust the prices based on your social skill.
    // cap adjustment so nothing is ever sold below value
    ///\EFFECT_INT_NPC slightly increases bartering price changes, relative to your INT

    ///\EFFECT_BARTER_NPC increases bartering price changes, relative to your BARTER

    ///\EFFECT_INT slightly increases bartering price changes, relative to NPC INT

    ///\EFFECT_BARTER increases bartering price changes, relative to NPC BARTER
    int const int_diff = seller.int_cur - buyer.int_cur;
    double const int_adj = 1 + 0.05 * std::min( 19, std::abs( int_diff ) );
    double const soc_adj = price_adjustment( round( seller.get_skill_level( skill_speech ) -
                           buyer.get_skill_level( skill_speech ) ) );
    double const adjust = int_diff >= 0 ? int_adj * soc_adj : soc_adj / int_adj;
    return seller.is_npc() ? adjust : -1 / adjust;
}

int npc_trading::bionic_install_price( Character &installer, Character &patient,
                                       item_location const &bionic )
{
    return bionic->price( true ) * 2 +
           ( bionic->is_owned_by( patient )
             ? 0
             : npc_trading::trading_price( patient, installer, { bionic, 1 } ) );
}

int npc_trading::adjusted_price( item const *it, int amount, Character const &buyer,
                                 Character const &seller )
{
    npc const *faction_party = buyer.is_npc() ? buyer.as_npc() : seller.as_npc();
    faction_price_rule const *const fpr = faction_party->get_price_rules( *it );

    double price = it->price_no_contents( true, fpr != nullptr ? fpr->price : std::nullopt );
    if( fpr != nullptr ) {
        price *= fpr->premium;
        if( seller.is_npc() ) {
            price *= fpr->markup;
        }
    }
    if( it->count_by_charges() && amount >= 0 ) {
        price *= static_cast<double>( amount ) / it->charges;
    }
    if( buyer.is_npc() ) {
        price = buyer.as_npc()->value( *it, price );
    } else if( seller.is_npc() ) {
        price = seller.as_npc()->value( *it, price );
    }

    if( fpr != nullptr && fpr->fixed_adj.has_value() ) {
        double const fixed_adj = fpr->fixed_adj.value();
        price *= 1 + ( seller.is_npc() ? fixed_adj : -fixed_adj );
    } else {
        double const adjust = npc_trading::net_price_adjustment( buyer, seller );
        price *= 1 + 0.25 * adjust;
    }

    return static_cast<int>( std::ceil( price ) );
}

namespace
{
int _trading_price( Character const &buyer, Character const &seller, item_location const &it,
                    int amount )
{
    if( seller.is_npc() ) {
        if( !seller.as_npc()->wants_to_sell( it, 1 ).success() ) {
            return 0;
        }
    } else if( buyer.is_npc() ) {
        if( !buyer.as_npc()->wants_to_buy( *it, 1 ).success() ) {
            return 0;
        }
    }
    int ret = npc_trading::adjusted_price( it.get_item(), amount, buyer, seller );
    for( item_pocket const *pk : it->get_all_standard_pockets() ) {
        for( item const *pkit : pk->all_items_top() ) {
            ret += _trading_price( buyer, seller, item_location{ it, const_cast<item *>( pkit ) },
                                   -1 );
        }
    }
    return ret;
}
} // namespace

int npc_trading::trading_price( Character const &buyer, Character const &seller,
                                trade_selector::entry_t const &it )
{
    return _trading_price( buyer, seller, it.first, it.second );
}

void item_pricing::set_values( int ip_count )
{
    const item *i_p = loc.get_item();
    is_container = i_p->is_container() || i_p->is_ammo_container();
    vol = i_p->volume();
    weight = i_p->weight();
    if( is_container || i_p->count() == 1 ) {
        count = ip_count;
    } else {
        charges = i_p->count();
        if( charges > 0 ) {
            price /= charges;
            vol /= charges;
            weight /= charges;
        } else {
            debugmsg( "item %s has zero or negative charges", i_p->typeId().str() );
        }
    }
}

// Returns how much the NPC will owe you after this transaction.
// You must also check if they will accept the trade.
int npc_trading::calc_npc_owes_you( const npc &np, int your_balance )
{
    // Friends don't hold debts against friends.
    if( np.will_exchange_items_freely() ) {
        return 0;
    }

    // If they're going to owe you more than before, and it's more than they're willing
    // to owe, then cap the amount owed at the present level or their willingness to owe
    // (whichever is bigger).
    //
    // When could they owe you more than max_willing_to_owe? It could be from quest rewards,
    // when they were less angry, or from when you were better friends.
    if( your_balance > np.op_of_u.owed && your_balance > np.max_willing_to_owe() ) {
        return std::max( np.op_of_u.owed, np.max_willing_to_owe() );
    }

    // Fair's fair. NPC will remember this debt (or credit they've extended)
    return your_balance;
}
void npc_trading::update_npc_owed( npc &np, int your_balance, int your_sale_value )
{
    np.op_of_u.owed = calc_npc_owes_you( np, your_balance );
    np.op_of_u.sold += your_sale_value;
}

// Oh my aching head
// op_of_u.owed is the positive when the NPC owes the player, and negative if the player owes the
// NPC
// cost is positive when the player owes the NPC money for a service to be performed
bool npc_trading::trade( npc &np, int cost, const std::string &deal )
{
    np.shop_restock();
    //np.drop_items( np.weight_carried() - np.weight_capacity(),
    //               np.volume_carried() - np.volume_capacity() );
    np.drop_invalid_inventory();

    std::unique_ptr<trade_ui> tradeui = std::make_unique<trade_ui>( get_avatar(), np, cost, deal );
    trade_ui::trade_result_t trade_result = tradeui->perform_trade();

    if( trade_result.traded ) {
        tradeui.reset();

        std::list<item_location *> from_map;

        std::list<item> escrow;
        avatar &player_character = get_avatar();
        // Movement of items in 3 steps: player to escrow - npc to player - escrow to npc.
        escrow = npc_trading::transfer_items( trade_result.items_you, player_character, np, from_map,
                                              true );
        npc_trading::transfer_items( trade_result.items_trader, np, player_character, from_map, false );
        // Now move items from escrow to the npc. Keep the weapon wielded.
        if( np.is_shopkeeper() ) {
            distribute_items_to_npc_zones( escrow, np );
        } else {
            for( const item &i : escrow ) {
                np.i_add( i, true, nullptr, nullptr, true, false );
            }
        }

        for( item_location *loc_ptr : from_map ) {
            if( !loc_ptr ) {
                continue;
            }
            item *it = loc_ptr->get_item();
            if( !it ) {
                continue;
            }
            if( it->has_var( "trade_charges" ) && it->count_by_charges() ) {
                it->charges -= static_cast<int>( it->get_var( "trade_charges", 0 ) );
                if( it->charges <= 0 ) {
                    loc_ptr->remove_item();
                } else {
                    it->erase_var( "trade_charges" );
                }
            } else if( it->has_var( "trade_amount" ) ) {
                loc_ptr->remove_item();
            }
        }

        // NPCs will remember debts, to the limit that they'll extend credit or previous debts
        if( !np.will_exchange_items_freely() ) {
            player_character.cash -= trade_result.delta_bank;
            update_npc_owed( np, trade_result.balance, trade_result.value_you );
            player_character.practice( skill_speech, trade_result.value_you / 10000 );
        }
    }
    return trade_result.traded ;
}

// Will the NPC accept the trade that's currently on offer?
bool npc_trading::npc_will_accept_trade( npc const &np, int your_balance )
{
    return np.will_exchange_items_freely() || your_balance + np.max_credit_extended() >= 0;
}
bool npc_trading::npc_can_fit_items( npc const &np, trade_selector::select_t const &to_trade )
{
    std::vector<item> avail_pockets = np.worn.available_pockets();

    if( !to_trade.empty() && avail_pockets.empty() ) {
        return false;
    }
    for( trade_selector::entry_t const &it : to_trade ) {
        bool item_stored = false;
        for( item &pkt : avail_pockets ) {
            const units::volume pvol = pkt.max_containable_volume();
            const item &i = *it.first;
            if( pkt.can_holster( i ) || ( pkt.can_contain( i ).success() && pvol > i.volume() ) ) {
                pkt.put_in( i, pocket_type::CONTAINER );
                item_stored = true;
                break;
            }
        }
        if( !item_stored ) {
            return false;
        }
    }
    return true;
}
