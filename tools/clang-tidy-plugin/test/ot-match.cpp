// RUN: %check_clang_tidy -allow-stdinc %s cata-ot-match %t -- --load=%cata_plugin -- -isystem %cata_include -isystem %cata_include/third-party

#include "enums.h"
#include "overmap.h"
#include "type_id.h"

const oter_id &ter();

void f0()
{
    is_ot_match( "ter0", ter(), ot_match_type::exact );
    // CHECK-MESSAGES: warning: Call to is_ot_match with fixed ot_match_type 'exact'.  For better efficientcy, instead compare the id directly. [cata-ot-match]
    // CHECK-FIXES: ( ter() == oter_str_id( "ter0" ).id() );
    is_ot_match( "ter1", ter(), ot_match_type::type );
    // CHECK-MESSAGES: warning: Call to is_ot_match with fixed ot_match_type 'type'.  For better efficientcy, instead compare the type directly. [cata-ot-match]
    // CHECK-FIXES: ( ( ter() )->get_type_id() == oter_type_str_id( "ter1" ) );
    is_ot_match( "ter2", ter(), ot_match_type::prefix );
    is_ot_match( "ter3", ter(), ot_match_type::contains );
}
