// RUN: %check_clang_tidy %s cata-ot-match %t -- -plugins=%cata_plugin -- -isystem %cata_include

struct oter_id;

const oter_id &ter();

enum class ot_match_type : int {
    exact,
    type,
    prefix,
    contains,
    num_ot_match_type
};

bool is_ot_match( const char *name, const oter_id &oter,
                  const ot_match_type match_type );

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
