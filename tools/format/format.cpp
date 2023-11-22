#include "format.h"
#include "json.h"

#include <functional>
#include <sstream>
#include <string>

static void write_array( TextJsonIn &jsin, JsonOut &jsout, int depth, bool force_wrap )
{
    jsout.start_array( force_wrap );
    jsin.start_array();
    while( !jsin.end_array() ) {
        formatter::format( jsin, jsout, depth );
    }
    jsout.end_array();
}

static void write_object( TextJsonIn &jsin, JsonOut &jsout, int depth, bool force_wrap )
{
    jsout.start_object( force_wrap );
    jsin.start_object();
    while( !jsin.end_object() ) {
        std::string name = jsin.get_member_name();
        jsout.member( name );
        bool override_wrap = false;
        if( name == "rows" || name == "blueprint" || name == "picture" ) {
            // Introspect into the row, if it has more than one element, force it to wrap.
            int in_start_pos = jsin.tell();
            bool ate_separator = jsin.get_ate_separator();
            {
                TextJsonArray arr = jsin.get_array();
                if( arr.size() > 1 ) {
                    override_wrap = true;
                }
            }
            jsin.seek( in_start_pos );
            jsin.set_ate_separator( ate_separator );
        }
        formatter::format( jsin, jsout, depth, override_wrap );
    }
    jsout.end_object();
}

static void format_collection( TextJsonIn &jsin, JsonOut &jsout, int depth,
                               const std::function<void( TextJsonIn &, JsonOut &, int, bool )> &write_func,
                               bool force_wrap )
{
    if( depth > 1 && !force_wrap ) {
        // We're backtracking by storing jsin and jsout state before formatting
        // and restoring it afterwards if necessary.
        int in_start_pos = jsin.tell();
        bool ate_separator = jsin.get_ate_separator();
        int out_start_pos = jsout.tell();
        bool need_separator = jsout.get_need_separator();
        write_func( jsin, jsout, depth, false );
        if( jsout.tell() - out_start_pos <= 120 ) {
            // Line is short enough, so we're done.
            return;
        } else {
            // Reset jsin and jsout to their initial state,
            // and we'll serialize while forcing wrapping.
            jsin.seek( in_start_pos );
            jsin.set_ate_separator( ate_separator );
            jsout.seek( out_start_pos );
            if( need_separator ) {
                jsout.set_need_separator();
            }
        }
    }
    write_func( jsin, jsout, depth, true );
}

void formatter::format( TextJsonIn &jsin, JsonOut &jsout, int depth, bool force_wrap )
{
    depth++;
    if( jsin.test_array() ) {
        format_collection( jsin, jsout, depth, write_array, force_wrap );
    } else if( jsin.test_object() ) {
        format_collection( jsin, jsout, depth, write_object, force_wrap );
    } else if( jsin.test_string() ) {
        // The string may contain escape sequences which we want to keep in the output.
        const int start_pos = jsin.tell();
        jsin.get_string();
        const int end_pos = jsin.tell();
        std::string str = jsin.substr( start_pos, end_pos - start_pos );
        str = str.substr( str.find( '"' ) );
        str = str.substr( 0, str.rfind( '"' ) + 1 );
        jsout.write_separator();
        *jsout.get_stream() << str;
        jsout.set_need_separator();
    } else if( jsin.test_number() ) {
        // Have to introspect into the string to distinguish integers from floats.
        // Otherwise they won't serialize correctly.
        const int start_pos = jsin.tell();
        jsin.skip_number();
        const int end_pos = jsin.tell();
        std::string str_form = jsin.substr( start_pos, end_pos - start_pos );
        jsin.seek( start_pos );
        if( str_form.find( '.' ) == std::string::npos ) {
            if( str_form.find( '-' ) == std::string::npos ) {
                const uint64_t num = jsin.get_uint64();
                jsout.write( num );
            } else {
                const int64_t num = jsin.get_int64();
                jsout.write( num );
            }
        } else {
            const double num = jsin.get_float();
            // This is QUITE insane, but as far as I can tell there is NO way to configure
            // an ostream to output a float/double meeting two constraints:
            // Always emit a decimal point (and single trailing 0 after a bare decimal point).
            // Don't emit trailing zeroes otherwise.
            std::string double_str = std::to_string( num );
            double_str.erase( double_str.find_last_not_of( '0' ) + 1, std::string::npos );
            if( double_str.back() == '.' ) {
                double_str += "0";
            }
            jsout.write_separator();
            *jsout.get_stream() << double_str;
            jsout.set_need_separator();
        }
    } else if( jsin.test_bool() ) {
        bool tf = jsin.get_bool();
        jsout.write( tf );
    } else if( jsin.test_null() ) {
        jsin.skip_null();
        jsout.write_null();
    } else {
        jsin.skip_value(); // this will throw exception with the invalid element
    }
}
