#include "bodygraph.h"

#include <imgui/imgui.h>
#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <set>

#include "bodypart.h"
#include "body_status_ui.h"
#include "cata_imgui.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character.h"
#include "character_attire.h"
#include "creature.h"
#include "damage.h"
#include "debug.h"
#include "enums.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "output.h"
#include "string_formatter.h"
#include "subbodypart.h"
#include "translation.h"
#include "translations.h"
#include "units.h"
#include "weather.h"

#define BPGRAPH_MAXROWS 20
#define BPGRAPH_MAXCOLS 40

static const flag_id json_flag_THERMOMETER( "THERMOMETER" );

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

void bodygraph::load( const JsonObject &jo, std::string_view )
{
    optional( jo, was_loaded, "parent_bodypart", parent_bp );
    optional( jo, was_loaded, "fill_sym", fill_sym );
    if( jo.has_string( "fill_color" ) ) {
        fill_color = color_from_string( jo.get_string( "fill_color" ) );
    }

    if( jo.has_string( "mirror" ) ) {
        mirror.reset();
        mandatory( jo, false, "mirror", mirror );
    } else if( !was_loaded || jo.has_array( "rows" ) ) {
        rows.clear();
        fill_rows.clear();
        for( const JsonValue jval : jo.get_array( "rows" ) ) {
            if( !jval.test_string() ) {
                jval.throw_error( "\"rows\" array must contain string values." );
            } else {
                rows.emplace_back( utf8_display_split( jval.get_string() ) );
            }
        }
        if( jo.has_array( "fill_rows" ) ) {
            for( const JsonValue jval : jo.get_array( "fill_rows" ) ) {
                if( !jval.test_string() ) {
                    jval.throw_error( "\"rows\" array must contain string values." );
                } else {
                    fill_rows.emplace_back( utf8_display_split( jval.get_string() ) );
                }
            }
        }
    }

    if( !was_loaded || jo.has_object( "parts" ) ) {
        parts.clear();
        for( const JsonMember memb : jo.get_object( "parts" ) ) {
            const std::string &sym = memb.name();
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

    if( jo.has_string( "label_fill" ) ) {
        label_fill = jo.get_string( "label_fill" );
    }

}

void bodygraph::finalize()
{
    if( rows.size() > BPGRAPH_MAXROWS ) {
        debugmsg( "body_graph \"%s\" defines more rows than the maximum (%d).", id.c_str(),
                  BPGRAPH_MAXROWS );
    }

    if( !fill_rows.empty() &&  fill_rows.size() != rows.size() ) {
        debugmsg( "body_graph \"%s\" defines a different number of fill_rows than rows (%d vs. %d).",
                  id.c_str(),
                  fill_rows.size(), rows.size() );
    }

    int width = -1;
    bool w_warned = false;
    for( size_t i = 0; i < rows.size(); i++ ) {
        std::vector<std::string> &r = rows[i];
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

        r.insert( r.begin(), ( BPGRAPH_MAXCOLS - w ) / 2, " " );
        r.insert( r.end(), BPGRAPH_MAXCOLS - r.size(), " " );

        if( ! fill_rows.empty() ) {
            std::vector<std::string> &fr = fill_rows[i];
            if( fr.size() != r.size() ) {
                debugmsg( "body_graph \"%s\" defines a different number of columns in fill_rows than in rows (%d vs. %d).",
                          id.c_str(),
                          fr.size(), w );
            }
            fr.insert( fr.begin(), ( BPGRAPH_MAXCOLS - w ) / 2, " " );
            fr.insert( fr.end(), BPGRAPH_MAXCOLS - fr.size(), " " );
        }

    }

    for( std::vector<std::vector<std::string>> temp_rows : {
             rows, fill_rows
         } ) {
        if( !temp_rows.empty() ) {
            for( int i = ( BPGRAPH_MAXROWS - temp_rows.size() ) / 2; i > 0; i-- ) {
                std::vector<std::string> r;
                r.insert( r.begin(), BPGRAPH_MAXCOLS, " " );
                temp_rows.insert( temp_rows.begin(), r );
            }

            for( int i = temp_rows.size(); i <= BPGRAPH_MAXROWS; i++ ) {
                std::vector<std::string> r;
                r.insert( r.begin(), BPGRAPH_MAXCOLS, " " );
                temp_rows.emplace_back( r );
            }
        }
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

static const bodygraph_id &get_bg_rows( const bodygraph_id &bgid )
{
    if( !!bgid->mirror ) {
        return get_bg_rows( bgid->mirror.value() );
    }
    // bodygraph_id is a string_id with stable storage; callers do not pass temporaries.
    // NOLINTNEXTLINE(bugprone-return-const-ref-from-parameter)
    return bgid;
}

void body_status_ui_impl::draw_graph_imgui( const std::optional<std::string>
        &bodypart_or_subpart_id )
{
    cataimgui::draw_colored_text( ui_parent->bs_uistate.prepare_bodygraph_text(
                                      bodypart_or_subpart_id ) );
}

void body_status_ui_impl::draw_info_imgui( const std::vector<std::string> &info_txt ) const
{
    for( const std::string &line : info_txt ) {
        if( line == "--" ) {
            ImGui::Separator();
        } else {
            cataimgui::draw_colored_text( line );
        }
    }
}

void body_status_uistate::prepare_partlist()
{
    partlist.clear();
    for( const auto &bgp : displayed_graph_id->parts ) {
        for( const bodypart_id &bid : bgp.second.bodyparts ) {

            partlist.emplace_back( bid, sub_bodypart_str_id::NULL_ID(),
                                   &bgp.second, displayed_character.has_part( bid, body_part_filter::equivalent ) );
        }
        for( const sub_bodypart_id &sid : bgp.second.sub_bodyparts ) {
            const bodypart_id bid = sid->parent.id();
            partlist.emplace_back( bid, sid.id(), &bgp.second,
                                   displayed_character.has_part( bid, body_part_filter::equivalent ) );
        }
    }
    std::sort( partlist.begin(), partlist.end(),
    []( const bodygraph_part_info & a, const bodygraph_part_info & b ) {
        if( !a.sbp_id.is_null() && !b.sbp_id.is_null() ) {
            return a.sbp_id->name.translated_lt( b.sbp_id->name );
        }
        if( a.bp_id->name.translated_ne( b.bp_id->name ) ) {
            return a.bp_id->name.translated_lt( b.bp_id->name );
        }
        if( a.sbp_id.is_null() != b.sbp_id.is_null() ) {
            return !a.sbp_id.is_null();
        }
        return a != b;
    } );
}

void body_status_uistate::prepare_infolist()
{
    // reset info for a new read
    info = bodygraph_info();
    info_txt.clear();

    const bodygraph_part_info &selected_part = get_selected_partlist();

    // if character doesn't have this part, don't generate info
    if( !selected_part.present_on_character ) {
        return;
    }

    const bodypart_id &bp = selected_part.bp_id;

    // sbps will either be a group for a body part and we'll need to do averages OR a single sub part
    std::set<sub_bodypart_id> sub_parts;

    // this might be null need to test for nullbp as this all continues
    if( selected_part.sbp_id != sub_bodypart_str_id::NULL_ID() ) {
        sub_parts.emplace( selected_part.sbp_id->id );
    } else {
        for( const sub_bodypart_str_id &sbp : bp->sub_parts ) {
            // don't worry about secondary sub parts would just make things confusing
            if( !sbp->secondary ) {
                sub_parts.emplace( sbp.id() );
            }
        }
    }

    displayed_character.worn.prepare_bodymap_info( info, bp, sub_parts, displayed_character );

    // update info text cache
    prepare_infotext( true );
}

void body_status_uistate::prepare_infotext( bool reset_pos )
{
    if( info.specific_sublimb ) {
        // parent part
        info_txt.emplace_back( string_format( "%s: %s", colorize( _( "Sub part of" ), c_magenta ),
                                              info.parent_bp_name ) );
    }
    // part health
    std::pair<std::string, nc_color> hpbar = get_hp_bar( info.part_hp_cur, info.part_hp_max );
    info_txt.emplace_back( string_format( "%s: %s", colorize( _( "Health" ), c_magenta ),
                                          colorize( hpbar.first, hpbar.second ) ) );
    // part wetness
    info_txt.emplace_back( string_format( "%s: %d%%", colorize( _( "Wetness" ), c_magenta ),
                                          static_cast<int>( info.wetness * 100.0f ) ) );
    // part temperature
    const bool temp_precise = displayed_character.cache_has_item_with( json_flag_THERMOMETER ) ||
                              displayed_character.has_flag( json_flag_THERMOMETER );
    const units::temperature temp = units::from_legacy_bodypart_temp( info.temperature.first );
    info_txt.emplace_back( string_format( "%s: %s", colorize( _( "Body temp" ), c_magenta ),
                                          temp_precise ? colorize( print_temperature( temp ),
                                                  info.temperature.second ) : info.temp_approx ) );
    info_txt.emplace_back( "--" );
    // part effects
    info_txt.emplace_back( string_format( "%s:", colorize( _( "Effects" ), c_magenta ) ) );
    for( const effect &eff : info.effects ) {
        if( eff.get_id()->is_show_in_info() ) {
            game_message_type rt = eff.get_id()->get_rating( eff.get_intensity() );
            info_txt.emplace_back( string_format( "  %s", colorize( eff.disp_name(),
                                                  rt == m_good ? c_green : rt == m_bad ? c_red : c_yellow ) ) );
        }
    }
    info_txt.emplace_back( "--" );
    // worn armor
    info_txt.emplace_back( string_format( "%s:", colorize( _( "Worn" ), c_magenta ) ) );
    for( const std::string &worn : info.worn_names ) {
        info_txt.emplace_back( string_format( "  %s", worn ) );
    }
    info_txt.emplace_back( "--" );
    // coverage
    info_txt.emplace_back( string_format( "%s: %d%%",
                                          colorize( info.specific_sublimb ? _( "Coverage" ) : _( "Coverage (Avg.)" ), c_magenta ),
                                          info.avg_coverage ) );
    info_txt.emplace_back( "--" );
    // Env protection
    info_txt.emplace_back(
        string_format(
            "%s: %d",
            colorize(
                info.specific_sublimb ? _( "Environmental protection" ) : _( "Environmental protection (Avg.)" ),
                c_magenta
            ),
            info.avg_env_protection
        )
    );
    info_txt.emplace_back( "--" );
    // encumbrance
    info_txt.emplace_back( string_format( "%s: %d", colorize( _( "Encumbrance" ), c_magenta ),
                                          info.total_encumbrance ) );
    info_txt.emplace_back( "--" );
    // protection
    info_txt.emplace_back( string_format( "%s:",
                                          colorize( info.specific_sublimb ? _( "Protection" ) : _( "Protection (Avg.)" ), c_magenta ) ) );
    std::string prot_legend = string_format( "%s %s %s", colorize( _( "worst" ), c_red ),
                              colorize( _( "median" ), c_yellow ), colorize( _( "best" ), c_light_green ) );
    info_txt.emplace_back( prot_legend );
    auto get_res_str = [&]( const damage_type_id & dt ) -> std::string {
        const std::string wval = string_format( "%4.1f", info.worst_case.type_resist( dt ) );
        const std::string mval = string_format( "%4.1f", info.median_case.type_resist( dt ) );
        const std::string bval = string_format( "%4.1f", info.best_case.type_resist( dt ) );
        std::string txt = string_format( "%s %s %s", colorize( wval, c_red ), colorize( mval, c_yellow ), colorize( bval, c_light_green ) );
        return txt;
    };
    auto get_env_str = [&]( const damage_type_id & dt ) -> std::string {
        return colorize( string_format( "    %5.2f", info.best_case.type_resist( dt ) ), c_white );
    };
    for( const damage_type &dt : damage_type::get_all() ) {
        if( info.best_case.type_resist( dt.id ) > 1 ) {
            info_txt.emplace_back( string_format( "  %s:", uppercase_first_letter( dt.name.translated() ) ) );
            info_txt.emplace_back( dt.env ? get_env_str( dt.id ) : get_res_str( dt.id ) );
        }
    }
}

std::vector<std::string> get_bodygraph_lines( const Character &u,
        const bodygraph_callback &fragment_cb, const bodygraph_id &id, int width, int height,
        std::string_view label )
{
    width = ( width <= 0 || width > BPGRAPH_MAXCOLS ) ? BPGRAPH_MAXCOLS : width;
    height = ( height <= 0 || height > BPGRAPH_MAXROWS ) ? BPGRAPH_MAXROWS : height;

    std::vector<std::string> ret;

    // Bodypart not present on character
    if( !!id->parent_bp && !u.has_part( *id->parent_bp, body_part_filter::equivalent ) ) {
        std::string txt = string_format( u.is_avatar() ?
                                         //~ 1$ = 2nd person pronoun (You), 2$ = body part (left arm)
                                         _( "%1$s do not have a %2$s." ) :
                                         //~ 1$ = name of character, 2$ = body part (left arm)
                                         _( "%1$s does not have a %2$s." ), u.disp_name( true ),
                                         id->parent_bp->obj().name.translated() );
        for( int y = 0; y < height; y++ ) {
            if( y == height / 2 ) {
                txt.insert( txt.begin(), center_text_pos( txt, 0, width ), ' ' );
                ret.emplace_back( colorize( txt, c_light_red ) );
            } else {
                ret.emplace_back( width, ' ' );
            }
        }
        return ret;
    }

    const bool hflip = !!id->mirror;
    const bodygraph_id &rid = get_bg_rows( id );
    for( int i = 0; static_cast<size_t>( i ) < rid->rows.size() && i < height; i++ ) {
        std::string ret_row;
        int j = hflip ? rid->rows[i].size() - 1 : 0;
        size_t label_i = 0;
        for( int x = 0 ; x < width && j < BPGRAPH_MAXCOLS && j >= 0; hflip ? j-- : j++, x++ ) {
            std::string sym = " ";
            const std::string &r = rid->rows[i][j];
            bool label_sym = false;
            if( !id->fill_rows.empty() && id->label_fill == id->fill_rows[i][j] ) {
                if( label_i < label.length() ) {
                    label_sym = true;
                    sym = label[label_i++];
                }
            } else {
                sym = id->fill_sym.empty() ? r : id->fill_sym;
            }
            auto iter = id->parts.find( r );
            const bodygraph_part *bgp = nullptr;
            if( iter != id->parts.end() ) {
                bgp = &iter->second;
                bool missing_section = true;
                for( const bodypart_id &bp : iter->second.bodyparts ) {
                    if( u.has_part( bp, body_part_filter::equivalent ) ) {
                        missing_section = false;
                    }
                }
                for( const sub_bodypart_id &sp : iter->second.sub_bodyparts ) {
                    if( u.has_part( sp->parent, body_part_filter::equivalent ) ) {
                        missing_section = false;
                    }
                }
                if( !label_sym ) {
                    sym = missing_section ? " " : ( id->fill_rows.empty() ? iter->second.sym : id->fill_rows[i][j] );
                }
            }
            if( rid->rows[i][j] == " " ) {
                sym = " ";
            }
            ret_row.append( fragment_cb( bgp, sym ) );
        }
        ret.emplace_back( ret_row );
    }
    return ret;
}

bool bodygraph_part::has_bodypart( const std::string &part ) const
{
    for( bodypart_id bpid : bodyparts ) {
        if( bpid == bodypart_id( part ) ) {
            return true;
        }
    }
    for( sub_bodypart_id bpid : sub_bodyparts ) {
        if( bpid == sub_bodypart_id( part ) ) {
            return true;
        }
    }
    return false;
}
