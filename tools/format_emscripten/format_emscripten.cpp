enum class error_log_format_t { human_readable };
extern constexpr error_log_format_t error_log_format = error_log_format_t::human_readable;

#include "json.h"
#include "format.h"

extern "C" {
    const char *json_format( const char *input, int colors )
    {
        json_error_output_colors = static_cast<json_error_output_colors_t>( colors );

        // we can do the malloc/free game with emscripten
        // or just leave the buffer in place and realloc
        // to make sure the strings fit
        static char *ret = nullptr;

        std::stringstream ss_out;
        std::stringstream ss_in( input );
        std::string ret_tmp;

        try {
            TextJsonIn jsin( ss_in );
            JsonOut jsout( ss_out, true );
            formatter::format( jsin, jsout );
            ret_tmp = ss_out.str();
        } catch( const std::exception &ex ) {
            ret_tmp = std::string( ex.what() );
        }

        const char *ret_ctmp = ret_tmp.c_str();
        const int len = strlen( ret_ctmp );
        ret = static_cast<char *>( realloc( ret, len + 1 ) );
        ret[0] = 0;
        strncat( ret, ret_ctmp, len );
        return ret;
    }
}
