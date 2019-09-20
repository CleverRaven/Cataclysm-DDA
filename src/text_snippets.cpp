#include "text_snippets.h"

#include <random>
#include <string>
#include <iterator>
#include <utility>

#include "json.h"
#include "rng.h"
#include "translations.h"

static const std::string null_string;

snippet_library SNIPPET;

snippet_library::snippet_library() = default;

void snippet_library::load_snippet( JsonObject &jsobj )
{
    const std::string category = jsobj.get_string( "category" );
    if( jsobj.has_array( "text" ) ) {
        JsonArray jarr = jsobj.get_array( "text" );
        add_snippets_from_json( category, jarr );
    } else {
        add_snippet_from_json( category, jsobj );
    }
}

void snippet_library::add_snippets_from_json( const std::string &category, JsonArray &jarr )
{
    while( jarr.has_more() ) {
        if( jarr.test_string() ) {
            const std::string text = jarr.next_string();
            add_snippet( category, text );
        } else {
            JsonObject jo = jarr.next_object();
            add_snippet_from_json( category, jo );
        }
    }
}

void snippet_library::add_snippet_from_json( const std::string &category, JsonObject &jo )
{
    const std::string text = jo.get_string( "text" );
    const int hash = add_snippet( category, text );
    if( jo.has_member( "id" ) ) {
        const std::string id = jo.get_string( "id" );
        snippets_by_id[id] = hash;
    }
}

int snippet_library::add_snippet( const std::string &category, const std::string &text )
{
    int hash = djb2_hash( reinterpret_cast<const unsigned char *>( text.c_str() ) );
    snippets.insert( std::pair<int, std::string>( hash, text ) );
    categories.insert( std::pair<std::string, int>( category, hash ) );
    return hash;
}

bool snippet_library::has_category( const std::string &category ) const
{
    return categories.lower_bound( category ) != categories.end();
}

void snippet_library::clear_snippets()
{
    snippets.clear();
    snippets_by_id.clear();
    categories.clear();
}

int snippet_library::get_snippet_by_id( const std::string &id ) const
{
    const auto it = snippets_by_id.find( id );
    if( it != snippets_by_id.end() ) {
        return it->second;
    }
    return 0;
}

int snippet_library::assign( const std::string &category ) const
{
    return assign( category, rng_bits() );
}

int snippet_library::assign( const std::string &category, const unsigned seed ) const
{
    const int count = categories.count( category );
    if( count == 0 ) {
        return 0;
    }
    std::mt19937 generator( seed );
    std::uniform_int_distribution<int> dis( 0, count - 1 );
    const int selected_text = dis( generator );
    std::multimap<std::string, int>::const_iterator it = categories.lower_bound( category );
    for( int index = 0; index < selected_text; ++index ) {
        ++it;
    }
    return it->second;
}

std::string snippet_library::get( const int index ) const
{
    const std::map<int, std::string>::const_iterator chosen_snippet = snippets.find( index );
    if( chosen_snippet == snippets.end() ) {
        return null_string;
    }
    return _( chosen_snippet->second );
}

std::string snippet_library::random_from_category( const std::string &cat ) const
{
    const auto iters = categories.equal_range( cat );
    if( iters.first == iters.second ) {
        return null_string;
    }

    const int count = std::distance( iters.first, iters.second );
    const int index = rng( 0, count - 1 );
    auto iter = iters.first;
    std::advance( iter, index );
    return get( iter->second );
}

std::vector<int> snippet_library::all_ids_from_category( const std::string &cat ) const
{
    std::vector<int> ret;
    const auto iters = categories.equal_range( cat );
    if( iters.first == categories.end() ) {
        return ret;
    }

    for( auto iter = iters.first; iter != iters.second; iter++ ) {
        ret.push_back( iter->second );
    }

    return ret;
}

