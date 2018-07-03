#include "basecamp.h"
#include <algorithm>
#include <sstream>

#include "output.h"
#include "translations.h"
#include "string_formatter.h"

basecamp::basecamp()
    : name(), posx( 0 ), posy( 0 )
{
}

basecamp::basecamp( std::string const &name_, int const posx_, int const posy_ )
    : name( name_ ), posx( posx_ ), posy( posy_ )
{
}

std::string basecamp::board_name() const
{
    //~ Name of a basecamp
    return string_format( _( "%s Board" ), name.c_str() );
}

std::string basecamp::save_data() const
{
    std::ostringstream data;

    // TODO: This will lose underscores, is that a problem?
    // strip spaces from name
    std::string savename = name;
    replace( savename.begin(), savename.end(), ' ', '_' );

    data << savename << " " << posx << " " << posy;
    return data.str();
}

void basecamp::load_data( std::string const &data )
{
    std::stringstream stream( data );
    stream >> name >> posx >> posy;

    // add space to name
    replace( name.begin(), name.end(), '_', ' ' );
}
