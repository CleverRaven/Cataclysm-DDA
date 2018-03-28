#include "uistate.h"
#include "item.h"
#include "json.h"

uistatedata uistate;

uistatedata::uistatedata()
{
}

void uistatedata::serialize( JsonOut &json ) const
{
    json.start_object();

    _serialize( json );

    for( auto &module : modules ) {
        module->serialize( json );
    }

    json.end_object();
}

void uistatedata::deserialize( JsonIn &jsin )
{
    int start = jsin.tell();
    auto jo = jsin.get_object();
    int end = jsin.tell();

    jsin.seek( start );
    _deserialize( jsin );

    for( auto &module : modules ) {
        jsin.seek( start );
        module->deserialize( jsin );
    }

    jsin.seek( end );
}
