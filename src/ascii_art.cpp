#include "ascii_art.h"

#include <set>

#include "assign.h"
#include "catacharset.h"
#include "debug.h"
#include "generic_factory.h"
#include "output.h"
#include "string_id.h"

static const int ascii_art_width = 42;

namespace
{
generic_factory<ascii_art> ascii_art_factory( "ascii_art" );
} // namespace

template<>
const ascii_art &string_id<ascii_art>::obj() const
{
    return ascii_art_factory.obj( *this );
}

template<>
bool string_id<ascii_art>::is_valid() const
{
    return ascii_art_factory.is_valid( *this );
}

void ascii_art::load_ascii_art( const JsonObject &jo, const std::string &src )
{
    ascii_art_factory.load( jo, src );
}

void ascii_art::load( const JsonObject &jo, const std::string & )
{
    assign( jo, "id", id );

    assign( jo, "picture", picture );
    for( std::string &line : picture ) {
        if( utf8_width( remove_color_tags( line ) ) > ascii_art_width ) {
            line = trim_by_length( line, ascii_art_width );
            debugmsg( "picture in %s contains a line too long to be displayed (>%i char).", id.c_str(),
                      ascii_art_width );
        }
    }
}

