#include <functional>
#include <iterator>
#include <string>

#include "cached_options.h"
#include "catacharset.h"
#include "debug.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "text_style_check.h"

text_style_check_reader::text_style_check_reader( const allow_object object_allowed )
    : object_allowed( object_allowed )
{
}

std::string text_style_check_reader::get_next( const JsonValue &jv ) const
{
    const JsonValue &jsin = jv;
    std::string raw;
    bool check_style = true;
    if( object_allowed == allow_object::yes && jsin.test_object() ) {
        JsonObject jsobj = jsin.get_object();
        raw = jsobj.get_string( "str" );
        if( jsobj.has_member( "//NOLINT(cata-text-style)" ) ) {
            check_style = false;
        }
    } else {
        raw = jsin.get_string();
    }
#ifndef CATA_IN_TOOL
    if( test_mode && check_style ) {
        const auto text_style_callback =
            [&jsin]
            ( const text_style_fix type, const std::string & msg,
              const std::u32string::const_iterator & beg, const std::u32string::const_iterator & /*end*/,
              const std::u32string::const_iterator & /*at*/,
              const std::u32string::const_iterator & from, const std::u32string::const_iterator & to,
              const std::string & fix
        ) {
            std::string err;
            switch( type ) {
                case text_style_fix::removal:
                    err = msg + "\n"
                          + "    Suggested fix: remove \"" + utf32_to_utf8( std::u32string( from, to ) ) + "\"\n"
                          + "    At the following position (marked with caret)";
                    break;
                case text_style_fix::insertion:
                    err = msg + "\n"
                          + "    Suggested fix: insert \"" + fix + "\"\n"
                          + "    At the following position (marked with caret)";
                    break;
                case text_style_fix::replacement:
                    err = msg + "\n"
                          + "    Suggested fix: replace \"" + utf32_to_utf8( std::u32string( from, to ) )
                          + "\" with \"" + fix + "\"\n"
                          + "    At the following position (marked with caret)";
                    break;
            }
            try {
                jsin.string_error( std::distance( beg, to ), err );
            } catch( const JsonError &e ) {
                debugmsg( "(json-error)\n%s", e.what() );
            }
        };

        const std::u32string raw32 = utf8_to_utf32( raw );
        text_style_check<std::u32string::const_iterator>( raw32.cbegin(), raw32.cend(),
                fix_end_of_string_spaces::yes, escape_unicode::no, text_style_callback );
    }
#endif
    return raw;
}
