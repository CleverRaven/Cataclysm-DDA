#pragma once
#ifndef ATM_MENU_H
#define ATM_MENU_H

#include "player.h"
#include "ui.h"
#include "item.h"


//--------------------------------------------------------------------------------------------------
//! Implements atm interactions
//--------------------------------------------------------------------------------------------------
class atm_menu
{
    public:
        // menu choices
        enum options : int {
            cancel, purchase_card, deposit_money, withdraw_money, transfer_all_money
        };

        atm_menu()                           = delete;
        atm_menu( atm_menu const & )            = delete;
        atm_menu( atm_menu && )                 = delete;
        atm_menu &operator=( atm_menu const & ) = delete;
        atm_menu &operator=( atm_menu && )      = delete;

        explicit atm_menu( player &p );        
        explicit atm_menu( player &p, item* it );

        void start();
    private:
        void add_choice( const int i, const char *const title );
        void add_info( const int i, const char *const title );

        options choose_option();

        //! Reset and repopulate the menu; with a fair bit of work this could be more efficient.
        void reset( const bool clear = true );

        //! print a bank statement for @p print = true;
        void finish_interaction( const bool print = true );

        //! Prompt for an integral value clamped to [0, max].
        static long prompt_for_amount( const char *const msg, const long max );

        //!Get a new cash card. $1.00 fine.
        bool do_purchase_card();

        //!Deposit money from cash card into bank account.
        bool do_deposit_money();

        //!Move money from bank account onto cash card.
        bool do_withdraw_money();

        //!Move the money from all the cash cards in inventory to a single card.
        bool do_transfer_all_money();

        player &u;
        bool portable;
        item *a;
        uilist amenu;
};

#endif
