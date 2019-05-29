#include "atm_menu.h"

#include "game.h"
#include "player.h"
#include "ui.h"
#include "messages.h"
#include "uistate.h"
#include "input.h"
#include "inventory.h"
#include "player_activity.h"
#include "calendar.h"
#include "item.h"
#include "output.h"
#include "string_input_popup.h"

//--------------------------------------------------------------------------------------------------
//! Implements atm interactions
//--------------------------------------------------------------------------------------------------
atm_menu::atm_menu( player &p ) : u( p ), portable( false )
{
    reset( false );
};

atm_menu::atm_menu( player &p, item *it ) : u( p ), portable( true ), a( it )
{
    reset( false );
}

void atm_menu::start()
{
    for( bool result = false; !result; ) {
        switch( choose_option() ) {
            case purchase_card:
                result = do_purchase_card();
                break;
            case deposit_money:
                result = do_deposit_money();
                break;
            case withdraw_money:
                result = do_withdraw_money();
                break;
            case transfer_all_money:
                result = do_transfer_all_money();
                break;
            default:
                return;
        }
        if( !u.activity.is_null() ) {
            break;
        }
        g->draw();
    }
}

void atm_menu::add_choice( const int i, const char *const title )
{
    amenu.addentry( i, true, -1, title );
}
void atm_menu::add_info( const int i, const char *const title )
{
    amenu.addentry( i, false, -1, title );
}

atm_menu::options atm_menu::choose_option()
{
    if( u.activity.id() == activity_id( "ACT_ATM" ) ) {
        return static_cast<options>( u.activity.index );
    }
    amenu.query();
    uistate.atm_selected = amenu.selected;
    return amenu.ret < 0 ? cancel : static_cast<options>( amenu.ret );
}

//! Reset and repopulate the menu; with a fair bit of work this could be more efficient.
void atm_menu::reset( const bool clear )
{
    const int card_count   = u.amount_of( "cash_card" );
    const int charge_count = card_count ? u.charges_of( "cash_card" ) : 0;

    if( clear ) {
        amenu.reset();
    }

    amenu.selected = uistate.atm_selected;
    amenu.text = string_format( _( "Welcome to the C.C.B.o.t.T. ATM. What would you like to do?\n"
                                   "Your current balance is: %s" ),
                                format_money( u.cash ) );

    if( !portable ) {
        if( u.cash >= 100 ) {
            add_choice( purchase_card, _( "Purchase cash card?" ) );
        } else {
            add_info( purchase_card, _( "You need $1.00 in your account to purchase a card." ) );
        }
    }

    if( card_count && u.cash > 0 ) {
        add_choice( withdraw_money, _( "Withdraw Money" ) );
    } else if( u.cash > 0 ) {
        add_info( withdraw_money, _( "You need a cash card before you can withdraw money!" ) );
    } else {
        add_info( withdraw_money,
                  _( "You need money in your account before you can withdraw money!" ) );
    }

    if( charge_count ) {
        add_choice( deposit_money, _( "Deposit Money" ) );
    } else {
        add_info( deposit_money,
                  _( "You need a charged cash card before you can deposit money!" ) );
    }

    if( card_count >= 2 && charge_count ) {
        add_choice( transfer_all_money, _( "Transfer All Money" ) );
    }
}

//! print a bank statement for @p print = true;
void atm_menu::finish_interaction( const bool print )
{
    if( print ) {
        add_msg( m_info, _( "Your account now holds %s." ), format_money( u.cash ) );
    }

    u.moves -= 100;
}

//! Prompt for an integral value clamped to [0, max].
long atm_menu::prompt_for_amount( const char *const msg, const long max )
{
    const std::string formatted = string_format( msg, max );
    const long amount = string_input_popup()
                        .title( formatted )
                        .width( 20 )
                        .text( to_string( max ) )
                        .only_digits( true )
                        .query_long();

    return ( amount > max ) ? max : ( amount <= 0 ) ? 0 : amount;
}

//!Get a new cash card. $1.00 fine.
bool atm_menu::do_purchase_card()
{
    const char *prompt = _( "This will automatically deduct $1.00 from your bank account. Continue?" );

    if( !query_yn( prompt ) ) {
        return false;
    }

    item card( "cash_card", calendar::turn );
    card.charges = 0;
    u.i_add( card );
    u.cash -= 100;
    u.moves -= 100;
    finish_interaction();

    return true;
}

//!Deposit money from cash card into bank account.
bool atm_menu::do_deposit_money()
{
    long money = u.charges_of( "cash_card" );

    if( !money ) {
        popup( _( "You can only deposit money from charged cash cards!" ) );
        return false;
    }

    const int amount = prompt_for_amount( ngettext(
            "Deposit how much? Max: %d cent. (0 to cancel) ",
            "Deposit how much? Max: %d cents. (0 to cancel) ", money ), money );

    if( !amount ) {
        return false;
    }

    add_msg( m_info, "amount: %d", amount );
    u.use_charges( "cash_card", amount );
    u.cash += amount;
    u.moves -= 100;
    finish_interaction();

    return true;
}

//!Move money from bank account onto cash card.
bool atm_menu::do_withdraw_money()
{
    //We may want to use visit_items here but thats fairly heavy.
    //For now, just check weapon if we didnt find it in the inventory.
    int pos = u.inv.position_by_type( "cash_card" );
    item *dst;
    if( pos == INT_MIN ) {
        dst = &u.weapon;
    } else {
        dst = &u.i_at( pos );
    }

    if( dst->is_null() ) {
        //Just in case we run into an edge case
        popup( _( "You do not have a cash card to withdraw money!" ) );
        return false;
    }

    const int amount = prompt_for_amount( ngettext(
            "Withdraw how much? Max: %d cent. (0 to cancel) ",
            "Withdraw how much? Max: %d cents. (0 to cancel) ", u.cash ), u.cash );

    if( !amount ) {
        return false;
    }

    dst->charges += amount;
    u.cash -= amount;
    u.moves -= 100;
    finish_interaction();

    return true;
}

//!Move the money from all the cash cards in inventory to a single card.
bool atm_menu::do_transfer_all_money()
{
    item *dst;
    if( u.activity.id() == activity_id( "ACT_ATM" ) ) {
        u.activity.set_to_null(); // stop for now, if required, it will be created again.
        dst = &u.i_at( u.activity.position );
        if( dst->is_null() || dst->typeId() != "cash_card" ) {
            return false;
        }
    } else {
        const int pos = u.inv.position_by_type( "cash_card" );

        if( pos == INT_MIN ) {
            return false;
        }
        dst = &u.i_at( pos );
    }

    for( auto &i : u.inv_dump() ) {
        if( i == dst || i->charges <= 0 || i->typeId() != "cash_card" ) {
            continue;
        }
        if( u.moves < 0 ) {
            // Money from `*i` could be transferred, but we're out of moves, schedule it for
            // the next turn. Putting this here makes sure there will be something to be
            // done next turn.
            u.assign_activity( activity_id( "ACT_ATM" ), 0, transfer_all_money, u.get_item_position( dst ) );
            if( portable && ( u.activity.targets.size() == 0 ) ) {
                u.activity.targets.push_back( item_location( u, a ) );
            }
            break;
        }

        if( portable ) {
            if( a->ammo_sufficient() ) {
                a->ammo_consume( a->ammo_required(), u.pos() );
            } else {
                u.add_msg_if_player( m_info, _( "Mini-ATM batteries are dead." ) );
                break;
            }
        }
        dst->charges += i->charges;
        i->charges =  0;
        u.moves    -= 10;
    }
    return true;
}
