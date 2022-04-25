#include "bodygraph.h"
#include "generic_factory.h"

namespace
{

generic_factory<bodygraph> bodygraph_factory( "bodygraph" );

} // namespace

/** @relates string_id */
template<>
const bodygraph &string_id<bodygraph>::obj() const
{
    return bodygraph_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<bodygraph>::is_valid() const
{
    return bodygraph_factory.is_valid( *this );
}

void bodygraph::load_bodygraphs( const JsonObject &jo, const std::string &src )
{
    bodygraph_factory.load( jo, src );
}

void bodygraph::reset()
{
    bodygraph_factory.reset();
}

const std::vector<bodygraph> &bodygraph::get_all()
{
    return bodygraph_factory.get_all();
}

void bodygraph::finalize_all()
{
    bodygraph_factory.finalize();
}

void bodygraph::check_all()
{
    bodygraph_factory.check();
}

void bodygraph::load( const JsonObject &jo, const std::string & )
{
    optional( jo, was_loaded, "parent_bodypart", parent_bp );
    optional( jo, was_loaded, "fill_sym", fill_sym );
    if( jo.has_string( "fill_color" ) ) {
        fill_color = color_from_string( jo.get_string( "fill_color" ) );
    }

    if( !was_loaded || jo.has_array( "rows" ) ) {
        rows.clear();
        for( const JsonValue jval : jo.get_array( "rows" ) ) {
            if( !jval.test_string() ) {
                jval.throw_error( "\"rows\" array must contain string values." );
            } else {
                std::wstring wrow = utf8_to_wstr( jval.get_string() );
                std::vector<std::string> row;
                for( const wchar_t wc : wrow ) {
                    row.emplace_back( wstr_to_utf8( std::wstring( 1, wc ) ) );
                }
                rows.emplace_back( row );
            }
        }
    }

    if( !was_loaded || jo.has_object( "parts" ) ) {
        parts.clear();
        for( const JsonMember memb : jo.get_object( "parts" ) ) {
            const std::string sym = memb.name();
            JsonObject mobj = memb.get_object();
            bodygraph_part bpg;
            optional( mobj, false, "body_parts", bpg.bodyparts );
            optional( mobj, false, "sub_body_parts", bpg.sub_bodyparts );
            optional( mobj, false, "sym", bpg.sym, fill_sym );
            if( mobj.has_string( "select_color" ) ) {
                bpg.sel_color = color_from_string( mobj.get_string( "select_color" ) );
            } else {
                bpg.sel_color = fill_color;
            }
            optional( mobj, false, "nested_graph", bpg.nested_graph );
            parts.emplace( sym, bpg );
        }
    }
}

void bodygraph::finalize()
{
    if( rows.size() > BPGRAPH_MAXROWS ) {
        debugmsg( "body_graph \"%s\" defines more rows than the maximum (%d).", id.c_str(),
                  BPGRAPH_MAXROWS );
    }

    int width = -1;
    bool w_warned = false;
    for( std::vector<std::string> &r : rows ) {
        int w = r.size();
        if( width == -1 ) {
            width = w;
        } else if( !w_warned && width != w ) {
            debugmsg( "body_graph \"%s\" defines rows with different widths.", id.c_str() );
            w_warned = true;
        } else if( !w_warned && w > BPGRAPH_MAXCOLS ) {
            debugmsg( "body_graph \"%s\" defines rows with more columns than the maximum (%d).", id.c_str(),
                      BPGRAPH_MAXCOLS );
            w_warned = true;
        }
        for( int i = ( BPGRAPH_MAXCOLS - w ) / 2; i > 0; i-- ) {
            r.insert( r.begin(), " " );
        }
        for( int i = r.size(); i <= BPGRAPH_MAXCOLS; i++ ) {
            r.emplace_back( " " );
        }
    }

    for( int i = ( BPGRAPH_MAXROWS - rows.size() ) / 2; i > 0; i-- ) {
        std::vector<std::string> r;
        for( int j = 0; j < BPGRAPH_MAXCOLS; j++ ) {
            r.emplace_back( " " );
        }
        rows.insert( rows.begin(), r );
    }

    for( int i = rows.size(); i <= BPGRAPH_MAXROWS; i++ ) {
        std::vector<std::string> r;
        for( int j = 0; j < BPGRAPH_MAXCOLS; j++ ) {
            r.emplace_back( " " );
        }
        rows.emplace_back( r );
    }
}

void bodygraph::check() const
{
    if( parts.empty() ) {
        debugmsg( "body_graph \"%s\" defined without parts.", id.c_str() );
    }
    for( const auto &bgp : parts ) {
        if( utf8_width( bgp.first ) > 1 ) {
            debugmsg( "part \"%s\" in body_graph \"%s\" is more than 1 character.", bgp.first, id.c_str() );
        }
        if( bgp.second.bodyparts.empty() && bgp.second.sub_bodyparts.empty() ) {
            debugmsg( "part \"%s\" in body_graph \"%s\" contains no body_parts or sub_body_parts definitions.",
                      bgp.first, id.c_str() );
        }
    }
}
