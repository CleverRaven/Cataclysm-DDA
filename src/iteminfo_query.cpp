#include "iteminfo_query.h"

#include <vector>
#include <string>


iteminfo_query::iteminfo_query() : iteminfo_query_base()
{
};

iteminfo_query::iteminfo_query( const std::string &bits ) : iteminfo_query_base( bits )
{
};

iteminfo_query::iteminfo_query( const std::vector<iteminfo_parts> &setBits )
{
    for( auto &bit : setBits ) {
        set( static_cast<size_t>( bit ) );
    }
}

bool iteminfo_query::test( const iteminfo_parts &value ) const
{
    return iteminfo_query_base::test( static_cast<size_t>( value ) );
}

const iteminfo_query iteminfo_query::all = iteminfo_query(
            std::string( static_cast<size_t>( iteminfo_parts::MAX_VALUE ) + 1, '1' ) );

const iteminfo_query iteminfo_query::notext = iteminfo_query(
            std::string( static_cast<size_t>( iteminfo_parts::MAX_VALUE ) -
                         static_cast<size_t>( iteminfo_parts::RANGE_TEXT_END ),
                         '1' ) +
            std::string( static_cast<size_t>( iteminfo_parts::RANGE_TEXT_END ) -
                         static_cast<size_t>( iteminfo_parts::RANGE_TEXT_START ) + 1,
                         '0' ) +
            std::string( static_cast<size_t>( iteminfo_parts::RANGE_TEXT_START ), '1' ) );

const iteminfo_query iteminfo_query::anyflags = iteminfo_query(
std::vector<iteminfo_parts> {
    iteminfo_parts::DESCRIPTION_FLAGS,
    iteminfo_parts::DESCRIPTION_FLAGS_HELMETCOMPAT,
    iteminfo_parts::DESCRIPTION_FLAGS_FITS,
    iteminfo_parts::DESCRIPTION_FLAGS_VARSIZE,
    iteminfo_parts::DESCRIPTION_FLAGS_SIDED,
    iteminfo_parts::DESCRIPTION_FLAGS_POWERARMOR,
    iteminfo_parts::DESCRIPTION_FLAGS_POWERARMOR_RADIATIONHINT,
    iteminfo_parts::DESCRIPTION_IRRIDATION
} );
