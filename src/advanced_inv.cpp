#include "advanced_inv.h"

#include <string>  // for operator==, basic_string, string
#include <utility> // for move
#include <vector>  // for vector

#include "activity_type.h"     // for activity_id
#include "advuilist_helpers.h" // for add_aim_sources, setup_for_aim, iloc_...
#include "debug.h"             // for debugmsg
#include "transaction_ui.h"    // for transaction_ui<>::advuilist_t, transa...

void create_advanced_inv()
{
    using mytrui_t = transaction_ui<std::vector<advuilist_helpers::iloc_entry>>;
    mytrui_t mytrui( { 6, 3 } );
    advuilist_helpers::setup_for_aim<>( mytrui.left() );
    advuilist_helpers::setup_for_aim<>( mytrui.right() );
    advuilist_helpers::add_aim_sources<>( mytrui.left() );
    advuilist_helpers::add_aim_sources<>( mytrui.right() );
    // FIXME: maybe add an examine callback to advuilist?
    mytrui.setctxthandler( []( mytrui_t * /**/, std::string const &action ) {
        if( action == "EXAMINE" ) {
            debugmsg( "examine placeholder" );
        }
    } );
    mytrui.show();
}

void cancel_aim_processing()
{
    // uistate.transfer_save.re_enter_move_all = aim_entry::START;
}
