#include "item_search.h"

#include <array>
#include <cctype>
#include <map>
#include <memory>
#include <string_view>
#include <unordered_set>
#include <utility>

#include "avatar.h"
#include "bodypart.h"
#include "cata_utility.h"
#include "enums.h"
#include "flag.h"
#include "item.h"
#include "item_category.h"
#include "item_factory.h"
#include "itype.h"
#include "make_static.h"
#include "material.h"
#include "math_parser_type.h"
#include "requirements.h"
#include "ret_val.h"
#include "string_formatter.h"
#include "subbodypart.h"
#include "translation.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"

static std::pair<std::string, std::string> get_both( std::string_view a );

template<typename Unit>
static std::function< bool( const item & )> can_contain_filter( std::string_view hint,
        std::string_view filter, Unit max, std::vector<std::pair<std::string, Unit>> units,
        std::function<item( itype *, Unit u )> set_function )
{
    auto const error = [hint, filter]( char const *, size_t /* offset */ ) {
        throw math::runtime_error( _( string_format( hint, filter ) ) );
    };
    // Start at max. On convert failure: results are empty and user knows it is unusable.
    Unit uni = max;
    try {
        uni = detail::read_from_json_string_common<Unit>( filter, units, error );
    } catch( math::runtime_error &err ) {
        // TODO notify the user: popup( err.what() );
    }
    // copy the debug item template (itype), put it on heap so the itype pointer doesn't move
    // TODO unique_ptr
    std::shared_ptr<itype> filtered_fake_itype = std::make_shared<itype>
            ( *item_controller->find_template( STATIC( itype_id( "debug_item_search" ) ) ) );
    item filtered_fake_item = set_function( filtered_fake_itype.get(), uni );
    // pass to keep filtered_fake_itype valid until lambda capture is destroyed (while item is needed)
    return [filtered_fake_itype, filtered_fake_item]( const item & i ) {
        return i.can_contain( filtered_fake_item ).success();
    };
}

std::function<bool( const item & )> basic_item_filter( std::string filter )
{
    size_t colon;
    char flag = '\0';
    if( ( colon = filter.find( ':' ) ) != std::string::npos ) {
        if( colon >= 1 ) {
            flag = filter[colon - 1];
            filter = filter.substr( colon + 1 );
        }
    }
    switch( flag ) {
        // category
        case 'c':
            return [filter]( const item & i ) {
                return lcmatch( i.get_category_of_contents().name_header(), filter );
            };
        // material
        case 'm':
            return [filter]( const item & i ) {
                return std::any_of( i.made_of().begin(), i.made_of().end(),
                [&filter]( const std::pair<material_id, int> &mat ) {
                    return lcmatch( mat.first->name(), filter );
                } );
            };
        // qualities
        case 'q':
            return [filter]( const item & i ) {
                return i.type->has_any_quality( filter );
            };
        // both
        case 'b':
            return [filter]( const item & i ) {
                auto pair = get_both( filter );
                return item_filter_from_string( pair.first )( i )
                       && item_filter_from_string( pair.second )( i );
            };
        // disassembled components
        case 'd':
            return [filter]( const item & i ) {
                const auto &components = i.get_uncraft_components();
                for( const item_comp &component : components ) {
                    if( lcmatch( component.to_string(), filter ) ) {
                        return true;
                    }
                }
                return false;
            };
        // item notes
        case 'n':
            return [filter]( const item & i ) {
                const std::string note = i.get_var( "item_note" );
                return !note.empty() && lcmatch( note, filter );
            };
        // item flags, must type in whole flag string name(case insensitive) so as to avoid revealing hidden flags.
        case 'f':
            return [filter]( const item & i ) {
                std::string flag_filter = filter;
                transform( flag_filter.begin(), flag_filter.end(), flag_filter.begin(), ::toupper );
                const flag_id fsearch( flag_filter );
                if( fsearch.is_valid() ) {
                    return i.has_flag( fsearch );
                } else {
                    return false;
                }
            };
        // by book skill
        case 's':
            return [filter]( const item & i ) {
                if( get_avatar().has_identified( i.typeId() ) ) {
                    return lcmatch( i.get_book_skill(), filter );
                }
                return false;
            };
        // by can contain length
        case 'L': {
            return can_contain_filter<units::length>( "Failed to convert '%s' to length.\nValid examples:\n122 cm\n1101mm\n2   meter",
            filter, units::length::max(), units::length_units, []( itype * typ, units::length len ) {
                typ->longest_side = len;
                item itm( typ );
                itm.set_flag( flag_HARD );
                return itm;
            } );
        }
        // by can contain volume
        case 'V': {
            return can_contain_filter<units::volume>( "Failed to convert '%s' to volume.\nValid examples:\n750 ml\n4L",
            filter, units::volume::max(), units::volume_units, []( itype * typ, units::volume vol ) {
                typ->volume = vol;
                return item( typ );
            } );
        }
        // by can contain mass
        case 'M': {
            return can_contain_filter<units::mass>( "Failed to convert '%s' to mass.\nValid examples:\n12 mg\n400g\n25  kg",
            filter, units::mass::max(), units::mass_units, []( itype * typ, units::mass mas ) {
                typ->weight = mas;
                return item( typ );
            } );
        }
        // covers bodypart
        case 'v': {
            std::unordered_set<bodypart_id> filtered_bodyparts;
            std::unordered_set<sub_bodypart_id> filtered_sub_bodyparts;
            for( const body_part &bp : all_body_parts ) {
                const bodypart_str_id &bp_str_id = convert_bp( bp );
                if( lcmatch( body_part_name( bp_str_id, 1 ), filter )
                    || lcmatch( body_part_name( bp_str_id, 2 ), filter ) ) {
                    filtered_bodyparts.insert( bp_str_id->id );
                }
                for( const sub_bodypart_str_id &sbp : bp_str_id->sub_parts ) {
                    if( lcmatch( sbp->name.translated(), filter )
                        || lcmatch( sbp->name_multiple.translated(), filter ) ) {
                        filtered_sub_bodyparts.insert( sbp->id );
                    }
                }
            }
            return [filtered_bodyparts, filtered_sub_bodyparts]( const item & i ) {
                return std::any_of( filtered_bodyparts.begin(), filtered_bodyparts.end(),
                [&i]( const bodypart_id & bp ) {
                    return i.covers( bp );
                } )
                || std::any_of( filtered_sub_bodyparts.begin(), filtered_sub_bodyparts.end(),
                [&i]( const sub_bodypart_id & sbp ) {
                    return i.covers( sbp );
                } );
            };
        }
        // covers layer
        case 'e': {
            std::unordered_set<layer_level> filtered_layers;
            for( layer_level layer = layer_level( 0 ); layer != layer_level::NUM_LAYER_LEVELS; ++layer ) {
                if( lcmatch( item::layer_to_string( layer ), filter ) ) {
                    filtered_layers.insert( layer );
                }
            }
            return [filtered_layers]( const item & i ) {
                const std::vector<layer_level> layers = i.get_layer();
                return std::any_of( layers.begin(), layers.end(), [&filtered_layers]( layer_level l ) {
                    return filtered_layers.count( l );
                } );
            };
        }
        // by name
        default:
            return [filter]( const item & a ) {
                return lcmatch( remove_color_tags( a.tname() ), filter );
            };
    }
}

std::function<bool( const item & )> item_filter_from_string( const std::string &filter )
{
    return filter_from_string<item>( filter, basic_item_filter );
}

std::pair<std::string, std::string> get_both( std::string_view a )
{
    size_t split_mark = a.find( ';' );
    return std::pair( std::string( a.substr( 0, split_mark ) ),
                      std::string( a.substr( split_mark + 1 ) ) );
}
